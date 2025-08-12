/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_AVRCP_SUPPORT == 1)
#include <string.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "mpa.h"
#include "avrcp.h"
#include "avctp.h"
#include "obex.h"
#include "sys_timer.h"

#define AVRCP_HDR_LENGTH              3

#define AVRCP_CATEGORY_1              0x01
#define AVRCP_CATEGORY_2              0x02
#define AVRCP_CATEGORY_3              0x04
#define AVRCP_CATEGORY_4              0x08

#define OPCODE_UNIT_INFO              0x30
#define OPCODE_SUB_UNIT_INFO          0x31
#define OPCODE_PASS_THROUGH           0x7C
#define OPCODE_VENDOR_DEPENDENT       0x00

/** user defined for record cmd*/
#define PDU_ID_UNIT_INFO              0xF0
#define PDU_ID_PASS_THROUGH           0xF1

#define PASSTHROUGH_STATE_PRESSED     0
#define PASSTHROUGH_STATE_RELEASED    1

#define AVRCP_OP_ID_VENDOR_UNIQUE     0x7E

#define AVRCP_PKT_TYPE_NON_FRAGMENTED 0x00
#define AVRCP_PTK_TYPE_START          0x01
#define AVRCP_PKT_TYPE_CONTINUE       0x02
#define AVRCP_PKT_TYPE_END            0x03

/** AVRCP ctype */
#define AVRCP_CONTROL                 0x00
#define AVRCP_STATUS                  0x01
#define AVRCP_SPECIFIC_INQUIRY        0x02
#define AVRCP_NOTIFY                  0x03
#define AVRCP_GENERAL_INQUIRY         0x04

/** AVRCP Response */
#define AVRCP_NOT_IMPLEMENTED         0x08
#define AVRCP_ACCEPTED                0x09
#define AVRCP_REJECTED                0x0A
#define AVRCP_IN_TRANSITION           0x0B
#define AVRCP_IMPLEMENTED             0x0C
#define AVRCP_CHANGED                 0x0D
#define AVRCP_INTERIM                 0x0F

#define AVRCP_COVER_ART_TARGET_LEN          0x13
#define AVRCP_COVER_ART_TYPE_LEN            0x10
#define AVRCP_IMAGE_HANDLE_HEADER_ID        0x30
#define AVRCP_IMAGE_DESCRIPTION_HEADER_ID   0x71
#define AVRCP_IMAGE_HANDLE_LEN              0x10

#define AVRCP_MAX_QUEUED_CMD 10 //Should be larger than local supported CT events number

typedef struct t_avrcp_continuation
{
    uint8_t *p_buf;
    uint16_t write_index;
    uint16_t recombine_length;
} T_AVRCP_CONTINUATION;

typedef struct t_avrcp_fragmentation
{
    uint8_t *p_buf;
    uint8_t  pdu_id;
    uint16_t read_index;
    uint16_t total_length;
} T_AVRCP_FRAGMENTATION;

typedef enum t_avrcp_state
{
    AVRCP_STATE_DISCONNECTED  = 0x00,
    AVRCP_STATE_ALLOCATED     = 0x01,
    AVRCP_STATE_CONNECTING    = 0x02,
    AVRCP_STATE_CONNECTED     = 0x03,
    AVRCP_STATE_DISCONNECTING = 0x04,
} T_AVRCP_STATE;

typedef enum t_avrcp_cover_art_state
{
    AVRCP_COVER_ART_STATE_DISCONNECTED  = 0x00,
    AVRCP_COVER_ART_STATE_CONNECTING    = 0x01,
    AVRCP_COVER_ART_STATE_CONNECTED     = 0x02,
    AVRCP_COVER_ART_STATE_DISCONNECTING = 0x03,
    AVRCP_COVER_ART_STATE_GET_IMAGE     = 0x04,
    AVRCP_COVER_ART_STATE_GET_THUMBNAIL = 0x05,
} T_AVRCP_COVER_ART_STATE;

typedef struct
{
    T_OS_QUEUE            cmd_queue;
    T_AVRCP_STATE         state;
    uint16_t              cid;
    uint8_t               cmd_credits;
    uint8_t               transact_label;
    T_AVRCP_CONTINUATION  recombine;
    T_AVRCP_FRAGMENTATION fragment;
    uint16_t              remote_mtu;
    uint8_t               vol_change_pending_transact;
    uint8_t               play_status_change_pending_transact;
    uint8_t               track_change_pending_transact;
    uint8_t               addressed_player_change_pending_transact;
    uint8_t               get_element_attr_pending_transact;
    uint8_t               get_play_status_pending_transact;
    uint8_t               set_addressed_player_pending_transact;
    uint8_t               vendor_cmd_transact;
    T_SYS_TIMER_HANDLE    timer_handle;
    uint8_t               last_pdu_id;
} T_AVRCP_CTRL_CHANN;

typedef struct
{
    T_OS_QUEUE            cmd_queue;
    T_AVRCP_STATE         state;
    uint16_t              cid;
    uint8_t               cmd_credits;
    uint8_t               transact_label;
    uint8_t               pending_transact;
} T_AVRCP_BROWSING_CHANN;

typedef struct t_avrcp_cmd
{
    struct T_AVRCP_CMD          *p_next;
    uint8_t                      pdu_id;
    union
    {
        uint8_t                            capability_id;
        uint8_t                            event_id;
        uint8_t                            attr_id;
        uint8_t                            pdu_id;
        uint8_t                            volume;
        uint16_t                           player_id;
        T_AVRCP_REQ_GET_ELEMENT_ATTR       get_element_attrs;
        T_AVRCP_REQ_PASS_THROUGH           pass_through;
        T_AVRCP_REQ_GET_APP_SETTING_VALUE  get_app_setting_value;
        T_AVRCP_REQ_SET_APP_SETTING_VALUE  set_app_setting_value;
        T_AVRCP_REQ_PLAY_ITEM              play_item;
    } cmd_param;
} T_AVRCP_CMD;

typedef struct
{
    struct T_AVRCP_BROWSING_CMD        *p_next;
    uint8_t                             pdu_id;
    union
    {
        T_AVRCP_REQ_GET_FOLDER_ITEMS    get_folder_items;
        T_AVRCP_REQ_GET_ITEM_ATTRS      get_item_attrs;
        T_AVRCP_REQ_SEARCH              search;
        T_AVRCP_REQ_CHANGE_PATH         change_path;
        uint16_t                        player_id;
    } cmd_param;
} T_AVRCP_BROWSING_CMD;

typedef struct t_avrcp_link
{
    uint8_t                 bd_addr[6];
    T_AVRCP_CTRL_CHANN      ctrl_chann;
    T_AVRCP_BROWSING_CHANN  browsing_chann;
    T_OBEX_HANDLE           handle;
    T_AVRCP_COVER_ART_STATE cover_art_state;
} T_AVRCP_LINK;

typedef struct t_avrcp
{
    P_AVRCP_CBACK               cback;
    T_AVRCP_LINK                *link_list;
    uint32_t                    local_company_id;
    uint16_t                    wait_rsp_tout;
    uint8_t                     link_num;
    uint8_t                     ct_features;
    uint8_t                     tg_features;
} T_AVRCP;

T_AVRCP *p_avrcp;

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
const uint8_t avrcp_cover_art_target[AVRCP_COVER_ART_TARGET_LEN + 1] =
{
    AVRCP_COVER_ART_TARGET_LEN,//indicate total length
    OBEX_HI_TARGET,
    (uint8_t)(AVRCP_COVER_ART_TARGET_LEN >> 8),
    (uint8_t)AVRCP_COVER_ART_TARGET_LEN,
    //cover art uuid
    0x71, 0x63, 0xDD, 0x54, 0x4A, 0x7E, 0x11, 0xE2, 0xB4, 0x7C, 0x00, 0x50, 0xC2, 0x49, 0x00, 0x48
};

//NULL terminated ASCII : x-bt/img-img
const uint8_t image_type[AVRCP_COVER_ART_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(AVRCP_COVER_ART_TYPE_LEN >> 8),
    (uint8_t)AVRCP_COVER_ART_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x69, 0x6d, 0x67, 0x2d, 0x69, 0x6d, 0x67, 0x00
};

//NULL terminated ASCII : x-bt/img-thm
const uint8_t image_thumbnail_type[AVRCP_COVER_ART_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(AVRCP_COVER_ART_TYPE_LEN >> 8),
    (uint8_t)AVRCP_COVER_ART_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x69, 0x6d, 0x67, 0x2d, 0x74, 0x68, 0x6d, 0x00
};
#endif

void avrcp_tout_callback(T_SYS_TIMER_HANDLE handle);

T_AVRCP_LINK *avrcp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t      i;
    T_AVRCP_LINK *p_link = NULL;

    for (i = 0; i < p_avrcp->link_num; i++)
    {
        if (p_avrcp->link_list[i].ctrl_chann.state == AVRCP_STATE_DISCONNECTED)
        {
            p_link = &p_avrcp->link_list[i];
            p_link->ctrl_chann.timer_handle = sys_timer_create("avrcp_wait_rsp",
                                                               SYS_TIMER_TYPE_LOW_PRECISION,
                                                               (uint32_t)p_link,
                                                               p_avrcp->wait_rsp_tout * 1000,
                                                               false,
                                                               avrcp_tout_callback);
            if (p_link->ctrl_chann.timer_handle != NULL)
            {
                memcpy(p_link->bd_addr, bd_addr, 6);
                p_link->ctrl_chann.state = AVRCP_STATE_ALLOCATED;
                p_link->browsing_chann.state = AVRCP_STATE_ALLOCATED;
                break;
            }
            else
            {
                return NULL;
            }
        }
    }

    return p_link;
}

void avrcp_free_link(T_AVRCP_LINK *p_link)
{
    if (p_link != NULL)
    {
        PROFILE_PRINT_TRACE1("avrcp_free_link: bd_addr [%s]", TRACE_BDADDR(p_link->bd_addr));

        if (p_link->ctrl_chann.timer_handle != NULL)
        {
            sys_timer_delete(p_link->ctrl_chann.timer_handle);
        }

        if (p_link->ctrl_chann.recombine.p_buf != NULL) //recombination not completed, abort it
        {
            os_mem_free(p_link->ctrl_chann.recombine.p_buf);
        }

        memset(p_link, 0, sizeof(T_AVRCP_LINK));
    }
}

T_AVRCP_LINK *avrcp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t      i;
    T_AVRCP_LINK *p_link = NULL;

    if (bd_addr == NULL)
    {
        return NULL;
    }

    for (i = 0; i < p_avrcp->link_num; i++)
    {
        if (!memcmp(p_avrcp->link_list[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_avrcp->link_list[i];
            break;
        }
    }

    return p_link;
}

T_AVRCP_LINK *avrcp_find_link_by_cid(uint16_t cid)
{
    uint8_t      i;
    T_AVRCP_LINK *p_link = NULL;

    for (i = 0; i < p_avrcp->link_num; i++)
    {
        if (p_avrcp->link_list[i].ctrl_chann.cid == cid ||
            p_avrcp->link_list[i].browsing_chann.cid == cid)
        {
            p_link = &p_avrcp->link_list[i];
            break;
        }
    }

    return p_link;
}

bool avrcp_send_generic_cmd(T_AVRCP_LINK *p_link,
                            uint8_t      *avrcp_data,
                            uint16_t      length)
{
    if (p_link != NULL)
    {
        p_link->ctrl_chann.transact_label = (p_link->ctrl_chann.transact_label + 1) &
                                            0x0f; /* advance transaction label (4 bit field) */
        if (avctp_send_data(p_link->bd_addr, avrcp_data, length,
                            p_link->ctrl_chann.transact_label, AVCTP_MSG_TYPE_CMD))
        {
            p_link->ctrl_chann.cmd_credits = 0;
            sys_timer_start(p_link->ctrl_chann.timer_handle);
            return true;
        }
        else
        {
            PROFILE_PRINT_ERROR0("avrcp_send_generic_cmd failed");
        }
    }

    return false;
}

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
bool avrcp_browsing_send_generic_cmd(T_AVRCP_LINK *p_link,
                                     uint8_t      *avrcp_data,
                                     uint16_t      length)
{
    p_link->browsing_chann.transact_label = (p_link->browsing_chann.transact_label + 1) &
                                            0x0f; /* advance transaction label (4 bit field) */
    if (avctp_browsing_send_data2buf(p_link->bd_addr, avrcp_data, length,
                                     p_link->browsing_chann.transact_label, AVCTP_MSG_TYPE_CMD))
    {
        p_link->browsing_chann.cmd_credits = 0;
        return true;
    }
    else
    {
        PROFILE_PRINT_ERROR0("avrcp_browsing_send_generic_cmd: failed");
        return false;
    }
}

bool avrcp_browsing_send_generic_rsp(T_AVRCP_LINK *p_link,
                                     uint8_t      *avrcp_data,
                                     uint16_t      length)
{
    int32_t ret;

    if (p_link->browsing_chann.state != AVRCP_STATE_CONNECTED)
    {
        ret = 1;
        goto fail_check_state;
    }

    if (p_link->browsing_chann.pending_transact == 0xff)
    {
        ret = 2;
        goto fail_no_pending_transact;
    }

    if (avctp_browsing_send_data2buf(p_link->bd_addr, avrcp_data, length,
                                     p_link->browsing_chann.pending_transact,
                                     AVCTP_MSG_TYPE_RSP))
    {
        p_link->browsing_chann.pending_transact = 0xff;
    }
    else
    {
        ret = 3;
        goto fail_send_rsp;
    }

    return true;

fail_check_state:
fail_send_rsp:
fail_no_pending_transact:
    PROFILE_PRINT_ERROR1("avrcp_browsing_send_generic_rsp: failed %d", -ret);
    return false;
}
#endif

bool avrcp_send_vendor_BT_SIG_cmd(T_AVRCP_LINK *p_link,
                                  uint8_t       ctype,
                                  uint8_t       pdu_id,
                                  uint8_t      *para,
                                  uint16_t      para_length)
{
    PROFILE_PRINT_TRACE2("avrcp_send_vendor_BT_SIG_cmd: ctype 0x%02x,CONTROL pdu id 0x%02x",
                         ctype, pdu_id);

    if (p_link != NULL)
    {
        p_link->ctrl_chann.transact_label = (p_link->ctrl_chann.transact_label + 1) &
                                            0x0f; /* advance transaction label (4 bit field) */

        uint8_t avrcp_data[10]; //header before pdu para
        uint8_t *p = avrcp_data;

        *p++ = 0xf & ctype;
        *p++ = (AVRCP_SUBUNIT_TYPE_PANEL << 3) | (AVRCP_SUBUNIT_ID & 0x7);
        *p++ = OPCODE_VENDOR_DEPENDENT; //Operation Code
        *p++ = (uint8_t)(COMPANY_BT_SIG >> 16);
        *p++ = (uint8_t)(COMPANY_BT_SIG >> 8);
        *p++ = (uint8_t)COMPANY_BT_SIG;
        *p++ = pdu_id; //pdu id
        *p++ = AVRCP_PKT_TYPE_NON_FRAGMENTED & 0x3; //Packet type: Non-Fragmented
        *p++ = (uint8_t)(para_length >> 8);
        *p++ = (uint8_t)para_length;

        if (avctp_send_data2buf(p_link->bd_addr, avrcp_data, sizeof(avrcp_data), para,
                                para_length, p_link->ctrl_chann.transact_label, AVCTP_MSG_TYPE_CMD))
        {
            p_link->ctrl_chann.cmd_credits = 0;
            sys_timer_start(p_link->ctrl_chann.timer_handle);

            return true;
        }
        else
        {
            PROFILE_PRINT_ERROR0("avrcp_send_vendor_BT_SIG_cmd: failed");
        }
    }

    return false;
}

bool avrcp_send_cmd_pass_through(T_AVRCP_LINK *p_link,
                                 uint8_t       key,
                                 bool          pressed)
{
    PROFILE_PRINT_INFO2("avrcp_send_pass_through: key 0x%02x, press %d", key, pressed);

    if (p_link != NULL)
    {
        uint8_t avrcp_data[10];
        uint8_t *p = avrcp_data;

        *p++ = AVRCP_CONTROL; //command type
        *p++ = (AVRCP_SUBUNIT_TYPE_PANEL << 3) | (AVRCP_SUBUNIT_ID & 0x7); //subunit type << 3, subunit ID
        *p++ = OPCODE_PASS_THROUGH; //Operation Code
        if (key == AVRCP_PASS_THROUGH_NEXT_GROUP || key == AVRCP_PASS_THROUGH_PREVIOUS_GROUP)
        {
            *p++ = (pressed ? PASSTHROUGH_STATE_PRESSED : PASSTHROUGH_STATE_RELEASED) << 7 |
                   AVRCP_OP_ID_VENDOR_UNIQUE; //state flag << 7, operation ID
            *p++ = 0x5; //operation data length
            *p++ = (uint8_t)(COMPANY_BT_SIG >> 16);
            *p++ = (uint8_t)(COMPANY_BT_SIG >> 8);
            *p++ = (uint8_t)COMPANY_BT_SIG;
            *p++ = 0;
            *p++ = key;   //Vendor_unique_operation_id
        }
        else
        {
            *p++ = (pressed ? PASSTHROUGH_STATE_PRESSED : PASSTHROUGH_STATE_RELEASED) << 7 |
                   (key & 0x7f); //state flag << 7, operation ID
            *p++ = 0;
        }

        return avrcp_send_generic_cmd(p_link, avrcp_data, p - avrcp_data);
    }

    return false;
}

void avrcp_cmd_process(T_AVRCP_LINK *p_link)
{
    if (p_link->ctrl_chann.state == AVRCP_STATE_CONNECTED)
    {
        T_AVRCP_CMD *p_cmd;
        bool ret = false;

        p_cmd = (T_AVRCP_CMD *)p_link->ctrl_chann.cmd_queue.p_first;
        if (p_cmd != NULL)
        {
            if (p_link->ctrl_chann.cmd_credits == 0)
            {
                PROFILE_PRINT_ERROR1("avrcp_cmd_process failed: no credits, pdu_id=0x%02x",
                                     p_cmd->pdu_id);
                return;
            }

            switch (p_cmd->pdu_id)
            {
            case PDU_ID_GET_CAPABILITIES:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_STATUS, PDU_ID_GET_CAPABILITIES,
                                                       &p_cmd->cmd_param.capability_id, 1);
                }
                break;

            case PDU_ID_GET_ELEMENT_ATTRS:
                {
                    uint8_t pdu_para[9 + (MAX_ELEMENT_ATTR_NUM * 4)] = {0};
                    uint8_t pdu_length;
                    uint32_t attr_tmp;
                    uint8_t i;

                    pdu_length = 9 + 4 * p_cmd->cmd_param.get_element_attrs.attr_num;
                    pdu_para[8] = p_cmd->cmd_param.get_element_attrs.attr_num;

                    for (i = 0; i < p_cmd->cmd_param.get_element_attrs.attr_num; i++)
                    {
                        attr_tmp = p_cmd->cmd_param.get_element_attrs.attr_id[i];
                        pdu_para[9 + (i * 4)] = attr_tmp >> 24;
                        pdu_para[9 + (i * 4) + 1] = attr_tmp >> 16;
                        pdu_para[9 + (i * 4) + 2] = attr_tmp >> 8;
                        pdu_para[9 + (i * 4) + 3] = attr_tmp;
                    }

                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_STATUS, PDU_ID_GET_ELEMENT_ATTRS,
                                                       pdu_para, pdu_length);
                }
                break;

            case PDU_ID_GET_PLAY_STATUS:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_STATUS, PDU_ID_GET_PLAY_STATUS,
                                                       NULL, 0);
                }
                break;

            case PDU_ID_SET_ABSOLUTE_VOLUME:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_CONTROL, PDU_ID_SET_ABSOLUTE_VOLUME,
                                                       &p_cmd->cmd_param.volume, 1);
                }
                break;

            case PDU_ID_REGISTER_NOTIFICATION:
                {
                    uint8_t pdu_para[] =
                    {
                        p_cmd->cmd_param.event_id,
                        0, 0, 0, 0
                    };
                    if (p_cmd->cmd_param.event_id == EVENT_PLAYBACK_POS_CHANGED)
                    {
                        //Playback interval = 1s
                        pdu_para[4] = 1;
                    }

                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_NOTIFY, PDU_ID_REGISTER_NOTIFICATION,
                                                       pdu_para, sizeof(pdu_para));
                }
                break;

            case PDU_ID_LIST_APP_SETTING_ATTRS:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_STATUS, PDU_ID_LIST_APP_SETTING_ATTRS,
                                                       NULL, 0);
                }
                break;

            case PDU_ID_LIST_APP_SETTING_VALUES:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_STATUS, PDU_ID_LIST_APP_SETTING_VALUES,
                                                       &p_cmd->cmd_param.attr_id, 1);
                }
                break;

            case PDU_ID_GET_CURRENT_APP_SETTING_VALUE:
                {
                    uint8_t pdu_para[1 + MAX_APP_SETTING_ATTR_NUM] = {0};
                    uint8_t pdu_length;
                    uint8_t i;

                    pdu_length = 1 + p_cmd->cmd_param.get_app_setting_value.attr_num;
                    pdu_para[0] = p_cmd->cmd_param.get_app_setting_value.attr_num;

                    for (i = 0; i < p_cmd->cmd_param.get_app_setting_value.attr_num; i++)
                    {
                        pdu_para[1 + i] = p_cmd->cmd_param.get_app_setting_value.attr_id[i];
                    }

                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_STATUS, PDU_ID_GET_CURRENT_APP_SETTING_VALUE,
                                                       pdu_para, pdu_length);
                }
                break;

            case PDU_ID_SET_APP_SETTING_VALUE:
                {
                    uint8_t pdu_para[1 + (MAX_APP_SETTING_ATTR_NUM * 2)] = {0};
                    uint8_t pdu_length;
                    uint8_t i;

                    pdu_length = 1 + p_cmd->cmd_param.set_app_setting_value.attr_num * 2;
                    pdu_para[0] = p_cmd->cmd_param.set_app_setting_value.attr_num;

                    for (i = 0; i < p_cmd->cmd_param.set_app_setting_value.attr_num; i++)
                    {
                        pdu_para[i * 2 + 1] = p_cmd->cmd_param.set_app_setting_value.app_setting[i].attr;
                        pdu_para[i * 2 + 2] = p_cmd->cmd_param.set_app_setting_value.app_setting[i].value;
                    }

                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_CONTROL, PDU_ID_SET_APP_SETTING_VALUE,
                                                       pdu_para, pdu_length);
                }
                break;

            case PDU_ID_REQUEST_CONTINUE_RSP:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_CONTROL, PDU_ID_REQUEST_CONTINUE_RSP,
                                                       &p_cmd->cmd_param.pdu_id, 1);
                }
                break;

            case PDU_ID_ABORT_CONTINUE_RSP:
                {
                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_CONTROL, PDU_ID_ABORT_CONTINUE_RSP,
                                                       &p_cmd->cmd_param.pdu_id, 1);
                }
                break;

            case PDU_ID_SET_ADDRESSED_PLAYER:
                {
                    uint8_t avrcp_data[] =
                    {
                        (uint8_t)(p_cmd->cmd_param.player_id >> 8),
                        (uint8_t)p_cmd->cmd_param.player_id
                    };

                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_CONTROL, PDU_ID_SET_ADDRESSED_PLAYER,
                                                       avrcp_data, sizeof(avrcp_data));
                }
                break;

            case PDU_ID_PLAY_ITEM:
                {
                    uint8_t avrcp_data[] =
                    {
                        (uint8_t)p_cmd->cmd_param.play_item.scope_id,

                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 56),
                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 48),
                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 40),
                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 32),
                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 24),
                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 16),
                        (uint8_t)(p_cmd->cmd_param.play_item.uid >> 8),
                        (uint8_t)p_cmd->cmd_param.play_item.uid,

                        (uint8_t)(p_cmd->cmd_param.play_item.uid_counter >> 8),
                        (uint8_t)p_cmd->cmd_param.play_item.uid_counter
                    };

                    ret = avrcp_send_vendor_BT_SIG_cmd(p_link, AVRCP_CONTROL, PDU_ID_PLAY_ITEM,
                                                       avrcp_data, sizeof(avrcp_data));
                }
                break;

            case PDU_ID_UNIT_INFO:
                {
                    uint8_t avrcp_data[] =
                    {
                        AVRCP_STATUS, //command type
                        0xff, //subunit type << 3 , subunit ID
                        OPCODE_UNIT_INFO, //Operation Code
                        0xff, 0xff, 0xff, 0xff, 0xff
                    };

                    ret = avrcp_send_generic_cmd(p_link, avrcp_data, sizeof(avrcp_data));
                }
                break;

            case PDU_ID_PASS_THROUGH:
                {
                    ret = avrcp_send_cmd_pass_through(p_link, p_cmd->cmd_param.pass_through.key,
                                                      p_cmd->cmd_param.pass_through.pressed);
                }
                break;

            default:
                break;
            }

            if (ret == true)
            {
                if (p_cmd->pdu_id != PDU_ID_UNIT_INFO &&
                    p_cmd->pdu_id != PDU_ID_PASS_THROUGH)
                {
                    p_link->ctrl_chann.last_pdu_id = p_cmd->pdu_id;
                }

                p_link->ctrl_chann.cmd_credits = 0; //Wait command response
                os_queue_delete(&p_link->ctrl_chann.cmd_queue, p_cmd);
                os_mem_free(p_cmd);
            }
            else
            {
                //if send cmd failed, process next cmd
                PROFILE_PRINT_ERROR1("avrcp_cmd_process: command failed, pdu_id=0x%02x",
                                     p_cmd->pdu_id);

                os_queue_delete(&p_link->ctrl_chann.cmd_queue, p_cmd);
                os_mem_free(p_cmd);
                avrcp_cmd_process(p_link);
            }
        }
    }
}

bool avrcp_cmd_enqueue(T_AVRCP_LINK *p_link,
                       uint8_t       pdu_id,
                       void         *para)
{
    T_AVRCP_CMD *p_cmd = NULL;

    if (p_link != NULL)
    {
        if (p_link->ctrl_chann.cmd_queue.count == AVRCP_MAX_QUEUED_CMD)
        {
            PROFILE_PRINT_ERROR0("avrcp_cmd_enqueue: cmd queue full");
            goto fail_cmd_enqueue;
        }

        p_cmd = os_mem_zalloc2(sizeof(T_AVRCP_CMD));
        if (p_cmd == NULL)
        {
            PROFILE_PRINT_ERROR0("avrcp_cmd_enqueue: zalloc memory fail");
            goto fail_cmd_enqueue;
        }

        p_cmd->pdu_id = pdu_id;
        switch (pdu_id)
        {
        case PDU_ID_GET_CAPABILITIES:
            p_cmd->cmd_param.capability_id = *(uint8_t *)para;
            break;

        case PDU_ID_GET_ELEMENT_ATTRS:
            memcpy(&p_cmd->cmd_param.get_element_attrs, para, sizeof(T_AVRCP_REQ_GET_ELEMENT_ATTR));
            break;

        case PDU_ID_GET_PLAY_STATUS:
            break;

        case PDU_ID_SET_ABSOLUTE_VOLUME:
            p_cmd->cmd_param.volume = *(uint8_t *)para;
            break;

        case PDU_ID_REGISTER_NOTIFICATION:
            p_cmd->cmd_param.event_id = *(uint8_t *)para;
            break;

        case PDU_ID_LIST_APP_SETTING_ATTRS:
            break;

        case PDU_ID_LIST_APP_SETTING_VALUES:
            p_cmd->cmd_param.attr_id = *(uint8_t *)para;
            break;

        case PDU_ID_GET_CURRENT_APP_SETTING_VALUE:
            memcpy(&p_cmd->cmd_param.get_app_setting_value, para, sizeof(T_AVRCP_REQ_GET_APP_SETTING_VALUE));
            break;

        case PDU_ID_SET_APP_SETTING_VALUE:
            memcpy(&p_cmd->cmd_param.set_app_setting_value, para, sizeof(T_AVRCP_REQ_SET_APP_SETTING_VALUE));
            break;

        case PDU_ID_REQUEST_CONTINUE_RSP:
            p_cmd->cmd_param.pdu_id = *(uint8_t *)para;
            break;

        case PDU_ID_ABORT_CONTINUE_RSP:
            p_cmd->cmd_param.pdu_id = *(uint8_t *)para;
            break;

        case PDU_ID_SET_ADDRESSED_PLAYER:
            p_cmd->cmd_param.player_id = *(uint16_t *)para;
            break;

        case PDU_ID_PLAY_ITEM:
            memcpy(&p_cmd->cmd_param.play_item, para, sizeof(T_AVRCP_REQ_PLAY_ITEM));
            break;

        case PDU_ID_UNIT_INFO:
            break;

        case PDU_ID_PASS_THROUGH:
            memcpy(&p_cmd->cmd_param.pass_through, para, sizeof(T_AVRCP_REQ_PASS_THROUGH));
            break;

        default:
            goto fail_cmd_enqueue;
        }

        os_queue_in(&p_link->ctrl_chann.cmd_queue, p_cmd);

        avrcp_cmd_process(p_link);

        return true;
    }

fail_cmd_enqueue:
    if (p_cmd != NULL)
    {
        os_mem_free(p_cmd);
    }
    return false;
}

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
bool avrcp_browsing_send_cmd_get_folder_items(T_AVRCP_LINK                 *p_link,
                                              T_AVRCP_REQ_GET_FOLDER_ITEMS *p_cmd_para)
{
    if (p_link != NULL)
    {
        uint8_t avrcp_data[13 + (MAX_ELEMENT_ATTR_NUM * 4)] = {0};
        uint8_t avrcp_length = 13 + p_cmd_para->attr_count * 4;
        uint8_t *p = &avrcp_data[13];
        uint8_t i;

        avrcp_data[0] = PDU_ID_GET_FOLDER_ITEMS;
        avrcp_data[1] = (uint8_t)((avrcp_length - 3) >> 8);
        avrcp_data[2] = (uint8_t)(avrcp_length - 3);
        avrcp_data[3] = p_cmd_para->scope;
        avrcp_data[4] = (uint8_t)(p_cmd_para->start_item >> 24);
        avrcp_data[5] = (uint8_t)(p_cmd_para->start_item >> 16);
        avrcp_data[6] = (uint8_t)(p_cmd_para->start_item >> 8);
        avrcp_data[7] = (uint8_t)(p_cmd_para->start_item);
        avrcp_data[8] = (uint8_t)(p_cmd_para->end_item >> 24);
        avrcp_data[9] = (uint8_t)(p_cmd_para->end_item >> 16);
        avrcp_data[10] = (uint8_t)(p_cmd_para->end_item >> 8);
        avrcp_data[11] = (uint8_t)(p_cmd_para->end_item);
        avrcp_data[12] = p_cmd_para->attr_count;

        for (i = 0; i < p_cmd_para->attr_count; i++)
        {
            *p++ = (uint8_t)(p_cmd_para->attr_id[i] >> 24);
            *p++ = (uint8_t)(p_cmd_para->attr_id[i] >> 16);
            *p++ = (uint8_t)(p_cmd_para->attr_id[i] >> 8);
            *p++ = (uint8_t)(p_cmd_para->attr_id[i]);
        }

        return avrcp_browsing_send_generic_cmd(p_link, avrcp_data, avrcp_length);
    }

    return false;
}

bool avrcp_browsing_send_cmd_get_item_attrs(T_AVRCP_LINK               *p_link,
                                            T_AVRCP_REQ_GET_ITEM_ATTRS *p_cmd_para)
{
    if (p_link != NULL)
    {
        uint8_t avrcp_data[15 + (MAX_ELEMENT_ATTR_NUM * 4)] = {0};
        uint8_t avrcp_length = 15 + p_cmd_para->num_of_attr * 4;
        uint8_t *p = &avrcp_data[15];
        uint8_t i;

        avrcp_data[0] = PDU_ID_GET_ITEM_ATTRS;
        avrcp_data[1] = (uint8_t)((avrcp_length - 3) >> 8);
        avrcp_data[2] = (uint8_t)(avrcp_length - 3);
        avrcp_data[3] = p_cmd_para->scope;
        avrcp_data[4] = (uint8_t)(p_cmd_para->uid >> 56);
        avrcp_data[5] = (uint8_t)(p_cmd_para->uid >> 48);
        avrcp_data[6] = (uint8_t)(p_cmd_para->uid >> 40);
        avrcp_data[7] = (uint8_t)(p_cmd_para->uid >> 32);
        avrcp_data[8] = (uint8_t)(p_cmd_para->uid >> 24);
        avrcp_data[9] = (uint8_t)(p_cmd_para->uid >> 16);
        avrcp_data[10] = (uint8_t)(p_cmd_para->uid >> 8);
        avrcp_data[11] = (uint8_t)(p_cmd_para->uid);
        avrcp_data[12] = (uint8_t)(p_cmd_para->uid_counter >> 8);
        avrcp_data[13] = (uint8_t)(p_cmd_para->uid_counter);
        avrcp_data[14] = p_cmd_para->num_of_attr;

        for (i = 0; i < p_cmd_para->num_of_attr; i++)
        {
            *p++ = (uint8_t)(p_cmd_para->attr_id[i] >> 24);
            *p++ = (uint8_t)(p_cmd_para->attr_id[i] >> 16);
            *p++ = (uint8_t)(p_cmd_para->attr_id[i] >> 8);
            *p++ = (uint8_t)(p_cmd_para->attr_id[i]);
        }

        return avrcp_browsing_send_generic_cmd(p_link, avrcp_data, avrcp_length);
    }

    return false;
}

bool avrcp_browsing_send_cmd_search(T_AVRCP_LINK       *p_link,
                                    T_AVRCP_REQ_SEARCH *p_cmd_para)
{
    bool ret = false;

    if (p_link != NULL)
    {
        if (p_link->browsing_chann.state == AVRCP_STATE_CONNECTED)
        {
            uint8_t *avrcp_data = os_mem_alloc2((7 + p_cmd_para->length));
            if (avrcp_data == NULL)
            {
                PROTOCOL_PRINT_WARN0("avrcp_browsing_search: get memory fail");
                return false;
            }

            avrcp_data[0] = PDU_ID_SEARCH;
            avrcp_data[1] = (uint8_t)((p_cmd_para->length + 4) >> 8);
            avrcp_data[2] = (uint8_t)(p_cmd_para->length + 4);
            avrcp_data[3] = 0;
            avrcp_data[4] = 0x6A; //UTF-8
            avrcp_data[5] = (uint8_t)(p_cmd_para->length >> 8);
            avrcp_data[6] = (uint8_t)p_cmd_para->length;
            memcpy(&avrcp_data[7], p_cmd_para->p_search_str, p_cmd_para->length);

            ret = avrcp_browsing_send_generic_cmd(p_link, avrcp_data, 7 + p_cmd_para->length);

            os_mem_free(avrcp_data);
        }
    }

    return ret;
}

bool avrcp_browsing_send_cmd_set_browsed_player(T_AVRCP_LINK *p_link,
                                                uint16_t      player_id)
{
    if (p_link != NULL)
    {
        uint8_t avrcp_data[] =
        {
            PDU_ID_SET_BROWSED_PLAYER,
            0, 0x2,
            (uint8_t)(player_id >> 8), (uint8_t)player_id
        };

        return avrcp_browsing_send_generic_cmd(p_link, avrcp_data, sizeof(avrcp_data));
    }

    return false;
}

bool avrcp_browsing_send_cmd_change_path(T_AVRCP_LINK            *p_link,
                                         T_AVRCP_REQ_CHANGE_PATH *p_cmd_para)
{
    if (p_link != NULL)
    {
        uint8_t avrcp_data[] =
        {
            PDU_ID_CHANGE_PATH,
            0, 0xB,
            (uint8_t)(p_cmd_para->uid_counter >> 8), (uint8_t)p_cmd_para->uid_counter,
            p_cmd_para->direction,
            (uint8_t)(p_cmd_para->folder_uid >> 56),
            (uint8_t)(p_cmd_para->folder_uid >> 48),
            (uint8_t)(p_cmd_para->folder_uid >> 40),
            (uint8_t)(p_cmd_para->folder_uid >> 32),
            (uint8_t)(p_cmd_para->folder_uid >> 24),
            (uint8_t)(p_cmd_para->folder_uid >> 16),
            (uint8_t)(p_cmd_para->folder_uid >> 8),
            (uint8_t)(p_cmd_para->folder_uid),
        };

        return avrcp_browsing_send_generic_cmd(p_link, avrcp_data, sizeof(avrcp_data));
    }

    return false;
}

void avrcp_browsing_cmd_process(T_AVRCP_LINK *p_link)
{
    if (p_link != NULL && p_link->browsing_chann.state == AVRCP_STATE_CONNECTED)
    {
        if (p_link->ctrl_chann.cmd_credits == 0)
        {
            PROFILE_PRINT_ERROR0("avrcp_browsing_cmd_process failed, no credits");
        }
        else
        {
            T_AVRCP_BROWSING_CMD *p_cmd;
            bool ret = false;

            p_cmd = (T_AVRCP_BROWSING_CMD *)p_link->browsing_chann.cmd_queue.p_first;
            if (p_cmd != NULL)
            {
                switch (p_cmd->pdu_id)
                {
                case PDU_ID_GET_FOLDER_ITEMS:
                    ret = avrcp_browsing_send_cmd_get_folder_items(p_link,
                                                                   (T_AVRCP_REQ_GET_FOLDER_ITEMS *)&p_cmd->cmd_param.get_folder_items);
                    break;

                case PDU_ID_GET_ITEM_ATTRS:
                    ret = avrcp_browsing_send_cmd_get_item_attrs(p_link,
                                                                 (T_AVRCP_REQ_GET_ITEM_ATTRS *)&p_cmd->cmd_param.get_item_attrs);
                    break;

                case PDU_ID_SEARCH:
                    {
                        ret = avrcp_browsing_send_cmd_search(p_link,
                                                             (T_AVRCP_REQ_SEARCH *)&p_cmd->cmd_param.search);

                        if (p_cmd->cmd_param.search.p_search_str != NULL)
                        {
                            os_mem_free(p_cmd->cmd_param.search.p_search_str);
                            p_cmd->cmd_param.search.p_search_str = NULL;
                        }
                    }
                    break;

                case PDU_ID_SET_BROWSED_PLAYER:
                    ret = avrcp_browsing_send_cmd_set_browsed_player(p_link,
                                                                     p_cmd->cmd_param.player_id);
                    break;

                case PDU_ID_CHANGE_PATH:
                    ret = avrcp_browsing_send_cmd_change_path(p_link,
                                                              (T_AVRCP_REQ_CHANGE_PATH *)&p_cmd->cmd_param.change_path);
                    break;

                default:
                    break;
                }

                if (ret == true)
                {
                    p_link->browsing_chann.cmd_credits = 0; //Wait command response
                    os_queue_delete(&p_link->browsing_chann.cmd_queue, p_cmd);
                    os_mem_free(p_cmd);
                }
                else
                {
                    //if send cmd failed, process next cmd
                    PROFILE_PRINT_ERROR0("avrcp_browsing_cmd_process: command failed");

                    os_queue_delete(&p_link->browsing_chann.cmd_queue, p_cmd);
                    os_mem_free(p_cmd);
                    avrcp_browsing_cmd_process(p_link);
                }
            }
        }
    }
}

bool avrcp_browsing_cmd_enqueue(T_AVRCP_LINK *p_link,
                                uint8_t       pdu_id,
                                void         *para)
{
    T_AVRCP_BROWSING_CMD *p_cmd = NULL;

    if (p_link != NULL)
    {
        if (p_link->browsing_chann.cmd_queue.count == AVRCP_MAX_QUEUED_CMD)
        {
            PROFILE_PRINT_ERROR0("avrcp_browsing_cmd_enqueue: cmd queue full");
            goto fail_enqueue;
        }

        p_cmd = os_mem_zalloc2(sizeof(T_AVRCP_BROWSING_CMD));
        if (p_cmd == NULL)
        {
            PROFILE_PRINT_ERROR0("avrcp_browsing_cmd_enqueue: zalloc memory fail");
            goto fail_enqueue;
        }

        p_cmd->pdu_id = pdu_id;
        switch (pdu_id)
        {
        case PDU_ID_GET_FOLDER_ITEMS:
            memcpy(&p_cmd->cmd_param.get_folder_items, para, sizeof(T_AVRCP_REQ_GET_FOLDER_ITEMS));
            break;

        case PDU_ID_GET_ITEM_ATTRS:
            memcpy(&p_cmd->cmd_param.get_item_attrs, para, sizeof(T_AVRCP_REQ_GET_ITEM_ATTRS));
            break;

        case PDU_ID_SEARCH:
            {
                T_AVRCP_REQ_SEARCH *tmp = (T_AVRCP_REQ_SEARCH *)para;

                p_cmd->cmd_param.search.p_search_str = os_mem_alloc2(tmp->length);
                if (p_cmd->cmd_param.search.p_search_str == NULL)
                {
                    PROFILE_PRINT_ERROR0("avrcp_browsing_cmd_enqueue: get memory fail");
                    goto fail_enqueue;
                }

                p_cmd->cmd_param.search.length = tmp->length;
                memcpy(p_cmd->cmd_param.search.p_search_str, tmp->p_search_str, tmp->length);
            }
            break;

        case PDU_ID_SET_BROWSED_PLAYER:
            p_cmd->cmd_param.player_id = *(uint16_t *)para;
            break;

        case PDU_ID_CHANGE_PATH:
            memcpy(&p_cmd->cmd_param.change_path, para, sizeof(T_AVRCP_REQ_CHANGE_PATH));
            break;

        default:
            goto fail_enqueue;
        }

        os_queue_in(&p_link->browsing_chann.cmd_queue, p_cmd);

        avrcp_browsing_cmd_process(p_link);

        return true;
    }

fail_enqueue:
    if (p_cmd != NULL)
    {
        os_mem_free(p_cmd);
    }
    return false;
}
#endif

void avrcp_tout_callback(T_SYS_TIMER_HANDLE handle)
{
    T_AVRCP_MSG_ERR err;
    T_AVRCP_LINK *p_link;

    p_link = (void *)sys_timer_id_get(handle);

    if (p_link != NULL)
    {
        PROTOCOL_PRINT_TRACE1("avrcp_tout_callback: addr %s", TRACE_BDADDR(p_link->bd_addr));

        err = AVRCP_WAIT_RSP_TO;

        p_link->ctrl_chann.cmd_credits = 1;
        avrcp_cmd_process(p_link);

        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_ERR, (void *)&err);
    }
}

bool avrcp_send_vendor_cmd(uint8_t   bd_addr[6],
                           uint8_t   subunit_type,
                           uint8_t   subunit_id,
                           uint8_t   ctype,
                           uint32_t  company_id,
                           uint8_t  *p_pdu,
                           uint16_t  pdu_length)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t avrcp_data[] = //[6]header before pdu
    {
        ctype & 0xf,
        (subunit_type << 3) | (subunit_id & 0x7),
        OPCODE_VENDOR_DEPENDENT,
        (uint8_t)(company_id >> 16),
        (uint8_t)(company_id >> 8),
        (uint8_t)company_id
    };

    PROFILE_PRINT_INFO2("avrcp_send_vendor_cmd: ctype 0x%02x, company_id 0x%x", ctype, company_id);

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if (p_link->ctrl_chann.cmd_credits == 0)
    {
        ret = 2;
        goto fail_no_credits;
    }

    p_link->ctrl_chann.transact_label = (p_link->ctrl_chann.transact_label + 1) & 0x0f;
    if (avctp_send_data2buf(p_link->bd_addr, avrcp_data, sizeof(avrcp_data), p_pdu, pdu_length,
                            p_link->ctrl_chann.transact_label, AVCTP_MSG_TYPE_CMD))
    {
        p_link->ctrl_chann.cmd_credits = 0;
        sys_timer_start(p_link->ctrl_chann.timer_handle);
    }
    else
    {
        ret = 3;
        goto fail_send_cmd;
    }

    return true;

fail_send_cmd:
fail_no_credits:
fail_find_link:
    PROFILE_PRINT_ERROR1("avrcp_send_vendor_cmd: failed %d", -ret);
    return false;
}

bool avrcp_send_vendor_rsp(uint8_t   bd_addr[6],
                           uint8_t   subunit_type,
                           uint8_t   subunit_id,
                           uint8_t   response,
                           uint32_t  company_id,
                           uint8_t  *p_pdu,
                           uint16_t  pdu_length)
{
    T_AVRCP_LINK *p_link;
    int ret = 0;
    uint8_t avrcp_data[] = //[6]avrcp header before pdu
    {
        response & 0xf, //response type
        (subunit_type << 3) | (subunit_id & 0x7),
        OPCODE_VENDOR_DEPENDENT,
        (uint8_t)(company_id >> 16),
        (uint8_t)(company_id >> 8),
        (uint8_t)company_id
    };

    PROFILE_PRINT_INFO1("avrcp_send_vendor_rsp: company_id %x", company_id);

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if (p_link->ctrl_chann.vendor_cmd_transact == 0xff)
    {
        ret = 2;
        goto fail_no_pending_transact;
    }

    if (avctp_send_data2buf(p_link->bd_addr, avrcp_data, sizeof(avrcp_data),
                            p_pdu, pdu_length, p_link->ctrl_chann.vendor_cmd_transact,
                            AVCTP_MSG_TYPE_RSP))
    {
        p_link->ctrl_chann.vendor_cmd_transact = 0xff;
    }
    else
    {
        ret = 3;
        goto fail_send_cmd;
    }

    return true;

fail_send_cmd:
fail_no_pending_transact:
fail_find_link:
    PROFILE_PRINT_ERROR1("avrcp_send_vendor_rsp: failed %d", -ret);
    return false;
}

void avrcp_send_vendor_BT_SIG_rsp(T_AVRCP_LINK *p_link,
                                  uint8_t       transact,
                                  uint8_t       response,
                                  uint8_t       pdu_id,
                                  uint8_t      *para,
                                  uint16_t      para_length)
{
    uint8_t avrcp_data[10]; //[10]avrcp header before pdu para
    uint8_t pkt_type;
    uint16_t mtu_size;

    PROFILE_PRINT_TRACE1("avrcp_send_vendor_BT_SIG_rsp: CONTROL pdu id 0x%02x", pdu_id);

    mtu_size = p_link->ctrl_chann.remote_mtu - 13; // avctp header(3) + avrcp header before pdu para(10)

    if (para_length <= mtu_size)
    {
        if (p_link->ctrl_chann.fragment.read_index == 0)
        {
            pkt_type = AVRCP_PKT_TYPE_NON_FRAGMENTED;
        }
        else
        {
            pkt_type = AVRCP_PKT_TYPE_END;
        }
    }
    else
    {
        if (p_link->ctrl_chann.fragment.read_index == 0)
        {
            pkt_type = AVRCP_PTK_TYPE_START;
        }
        else
        {
            pkt_type = AVRCP_PKT_TYPE_CONTINUE;
        }

        p_link->ctrl_chann.fragment.pdu_id = pdu_id;
        p_link->ctrl_chann.fragment.read_index += mtu_size;
        para_length = mtu_size;
    }

    avrcp_data[0] = response & 0xf;
    avrcp_data[1] = (AVRCP_SUBUNIT_TYPE_PANEL << 3) | (AVRCP_SUBUNIT_ID & 0x7);
    avrcp_data[2] = OPCODE_VENDOR_DEPENDENT;
    avrcp_data[3] = (uint8_t)(COMPANY_BT_SIG >> 16);
    avrcp_data[4] = (uint8_t)(COMPANY_BT_SIG >> 8);
    avrcp_data[5] = (uint8_t)COMPANY_BT_SIG;
    avrcp_data[6] = pdu_id;
    avrcp_data[7] = pkt_type & 0x3;
    avrcp_data[8] = (uint8_t)(para_length >> 8);
    avrcp_data[9] = (uint8_t)para_length;

    avctp_send_data2buf(p_link->bd_addr, avrcp_data, sizeof(avrcp_data),
                        para, para_length, transact, AVCTP_MSG_TYPE_RSP);

    if (pkt_type == AVRCP_PKT_TYPE_NON_FRAGMENTED ||
        pkt_type == AVRCP_PKT_TYPE_END)
    {
        if (p_link->ctrl_chann.fragment.p_buf != NULL)
        {
            os_mem_free(p_link->ctrl_chann.fragment.p_buf);
            memset(&p_link->ctrl_chann.fragment, 0, sizeof(p_link->ctrl_chann.fragment));
        }
    }
}

bool avrcp_send_unit_info(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_addr(bd_addr);
    PROFILE_PRINT_TRACE1("avrcp_send_unit_info: unif_info 0x%02x", OPCODE_UNIT_INFO);

    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_UNIT_INFO, NULL);
    }

    return false;
}

bool avrcp_send_pass_through(uint8_t     bd_addr[6],
                             T_AVRCP_KEY key,
                             bool        pressed)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = key;
        tmp.pressed = pressed;

        return avrcp_cmd_enqueue(p_link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool avrcp_get_capability(uint8_t bd_addr[6],
                          uint8_t capability_id)
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_GET_CAPABILITIES, &capability_id);
    }

    return false;
}

bool avrcp_get_play_status(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_addr(bd_addr);

    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_GET_PLAY_STATUS, NULL);
    }

    return false;
}

bool avrcp_get_element_attr(uint8_t  bd_addr[6],
                            uint8_t  attr_num,
                            uint8_t *p_attr)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_GET_ELEMENT_ATTR tmp;
        uint8_t i;

        tmp.attr_num = attr_num;
        for (i = 0; i < attr_num; i++)
        {
            tmp.attr_id[i] = p_attr[i];
        }

        return avrcp_cmd_enqueue(p_link, PDU_ID_GET_ELEMENT_ATTRS, &tmp);
    }

    return false;
}

bool avrcp_register_notification(uint8_t bd_addr[6],
                                 uint8_t event_id)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if ((event_id != EVENT_VOLUME_CHANGED) || (p_avrcp->ct_features & AVRCP_CATEGORY_2))
        {
            return avrcp_cmd_enqueue(p_link, PDU_ID_REGISTER_NOTIFICATION, &event_id);
        }
    }

    return false;
}

void avrcp_handle_vendor_BT_SIG_rsp(T_AVRCP_LINK *p_link,
                                    uint8_t      *p_pdu,
                                    uint16_t      pdu_length,
                                    uint8_t       response)
{
    uint8_t *p_data = p_pdu;
    uint8_t pdu_id = *p_data++;
    uint8_t packet_type = *p_data++ & 0x3;
    uint16_t para_length;

    BE_ARRAY_TO_UINT16(para_length, p_data);

    if (para_length)
    {
        p_data += 2;
    }

    if (packet_type == AVRCP_PKT_TYPE_NON_FRAGMENTED)
    {
        if (p_link->ctrl_chann.recombine.p_buf != NULL) //last recombination not completed, abort it
        {
            os_mem_free(p_link->ctrl_chann.recombine.p_buf);
            memset(&p_link->ctrl_chann.recombine, 0, sizeof(p_link->ctrl_chann.recombine));
        }
    }
    else //START,CONTINUE,END packet
    {
        uint8_t *buf_tmp;

        //AVRCP not define specific length of fragment packets
        //Re-allocate memory for each fragmentation
        p_link->ctrl_chann.recombine.recombine_length += para_length;
        buf_tmp = os_mem_alloc2(p_link->ctrl_chann.recombine.recombine_length);
        if (buf_tmp == NULL)
        {
            avrcp_cmd_enqueue(p_link, PDU_ID_ABORT_CONTINUE_RSP, &pdu_id);
            //PROTOCOL_PRINT_ERROR0("avrcp continuation aborted: malloc fail");
            return;
        }
        else
        {
            if (p_link->ctrl_chann.recombine.p_buf != NULL)
            {
                memcpy(buf_tmp, p_link->ctrl_chann.recombine.p_buf, p_link->ctrl_chann.recombine.write_index);
                os_mem_free(p_link->ctrl_chann.recombine.p_buf);
            }
            else //First fragment packet
            {
                if (packet_type != AVRCP_PTK_TYPE_START) /*recv CONTINUE/END without START*/
                {
                    avrcp_cmd_enqueue(p_link, PDU_ID_ABORT_CONTINUE_RSP, &pdu_id);
                    os_mem_free(buf_tmp);
                    PROFILE_PRINT_ERROR0("avrcp recv CONTINUE/END without START");
                    return;
                }
            }

            p_link->ctrl_chann.recombine.p_buf = buf_tmp;
        }

        memcpy(p_link->ctrl_chann.recombine.p_buf + p_link->ctrl_chann.recombine.write_index, p_data,
               para_length);
        p_link->ctrl_chann.recombine.write_index += para_length;

        if (packet_type != AVRCP_PKT_TYPE_END)
        {
            avrcp_cmd_enqueue(p_link, PDU_ID_REQUEST_CONTINUE_RSP, &pdu_id);
            return;
        }
        else
        {
            p_data = p_link->ctrl_chann.recombine.p_buf;
            para_length = p_link->ctrl_chann.recombine.write_index;
        }
    }

    //if (packet_type == AVRCP_PKT_TYPE_END)
    //{
    //    PROTOCOL_PRINT_INFO1("avrcp rx combined pdu: len=%d", p_link->recombine.write_index);
    //}

    PROFILE_PRINT_TRACE2("avrcp_handle_vendor_BT_SIG_rsp: packet_type 0x%02x, pdu_id 0x%02x",
                         packet_type, pdu_id);
    switch (pdu_id)
    {
    case PDU_ID_GET_CAPABILITIES:
        {
            T_RSP_CPBS tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.capability_id = *p_data++;
                tmp.capability_count = *p_data++;
                tmp.p_buf = p_data;
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_GET_CPBS, &tmp);
        }
        break;

    case PDU_ID_GET_ELEMENT_ATTRS:
        {
            T_RSP_GET_ELEMENT_ATTR *tmp = os_mem_alloc2(sizeof(T_RSP_GET_ELEMENT_ATTR));
            if (tmp == NULL)
            {
                //PROTOCOL_PRINT_WARN0("avrcp_handle_vendor_BT_SIG_rsp: get memory fail for element attrs");
                return;
            }

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp->state = AVRCP_RSP_STATE_SUCCESS;
                tmp->num_of_attr = *p_data++;

                if (tmp->num_of_attr > MAX_ELEMENT_ATTR_NUM)
                {
                    tmp->num_of_attr = MAX_ELEMENT_ATTR_NUM;
                }
                for (int i = 0; i < tmp->num_of_attr; i++)
                {
                    BE_ARRAY_TO_UINT32(tmp->attr[i].attribute_id, p_data);
                    p_data += 4;
                    BE_ARRAY_TO_UINT16(tmp->attr[i].character_set_id, p_data);
                    p_data += 2;
                    BE_ARRAY_TO_UINT16(tmp->attr[i].length, p_data);
                    p_data += 2;
                    tmp->attr[i].p_buf = p_data;
                    p_data += tmp->attr[i].length;
                }
            }
            else
            {
                tmp->state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_GET_ELEMENT_ATTR, tmp);
            os_mem_free(tmp);
        }
        break;

    case PDU_ID_GET_PLAY_STATUS:
        {
            T_RSP_GET_PLAY_STATUS tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                BE_ARRAY_TO_UINT32(tmp.length_ms, p_data);
                p_data += 4;
                BE_ARRAY_TO_UINT32(tmp.position_ms, p_data);
                p_data += 4;
                tmp.play_status = *p_data++;
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_GET_PLAYSTATUS, &tmp);
        }
        break;

    case PDU_ID_REGISTER_NOTIFICATION:
        {
            if (response == AVRCP_INTERIM)
            {
                T_RSP_REG_NOTIFICATION tmp;

                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.event_id = *p_data++;
                switch (tmp.event_id)
                {
                case EVENT_PLAYBACK_STATUS_CHANGED:
                    {
                        tmp.u.play_status = *p_data++;
                    }
                    break;

                case EVENT_TRACK_CHANGED:
                    {
                        uint64_t track_id;

                        BE_ARRAY_TO_UINT64(track_id, p_data);
                        p_data += 8;

                        tmp.u.track_id = track_id;
                    }
                    break;

                case EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
                    {
                        uint8_t i;

                        tmp.u.app_settings.num_of_attr = *p_data++;
                        for (i = 0; i < tmp.u.app_settings.num_of_attr; i++)
                        {
                            tmp.u.app_settings.app_setting[i].attr = *p_data++;
                            tmp.u.app_settings.app_setting[i].value = *p_data++;
                        }
                    }
                    break;

                case EVENT_NOW_PLAYING_CONTENT_CHANGED:
                    break;

                case EVENT_AVAILABLE_PLAYERS_CHANGED:
                    break;

                case EVENT_ADDRESSED_PLAYER_CHANGED:
                    {
                        BE_ARRAY_TO_UINT16(tmp.u.addressed_player.player_id, p_data);
                        p_data += 2;
                        BE_ARRAY_TO_UINT16(tmp.u.addressed_player.uid_counter, p_data);
                        p_data += 2;
                    }
                    break;

                case EVENT_UIDS_CHANGED:
                    {
                        BE_ARRAY_TO_UINT16(tmp.u.uid_counter, p_data);
                        p_data += 2;
                    }
                    break;

                case EVENT_VOLUME_CHANGED:
                    {
                        tmp.u.volume = (*p_data++) & 0x7f;
                    }
                    break;

                default:
                    break;
                }

                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_REG_NOTIFICATION, &tmp);
            }
            else if (response == AVRCP_CHANGED)
            {
                T_NOTIF_CHANGED tmp;

                tmp.event_id = *p_data++;
                switch (tmp.event_id)
                {
                case EVENT_PLAYBACK_STATUS_CHANGED:
                    {
                        tmp.u.play_status = *p_data++;
                    }
                    break;

                case EVENT_TRACK_CHANGED:
                    {
                        uint64_t track_id;

                        BE_ARRAY_TO_UINT64(track_id, p_data);
                        p_data += 8;

                        tmp.u.track_id = track_id;
                    }
                    break;

                case EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
                    {
                        uint8_t i;

                        tmp.u.app_settings.num_of_attr = *p_data++;
                        for (i = 0; i < tmp.u.app_settings.num_of_attr; i++)
                        {
                            tmp.u.app_settings.app_setting[i].attr = *p_data++;
                            tmp.u.app_settings.app_setting[i].value = *p_data++;
                        }
                    }
                    break;

                case EVENT_NOW_PLAYING_CONTENT_CHANGED:
                    break;

                case EVENT_AVAILABLE_PLAYERS_CHANGED:
                    break;

                case EVENT_ADDRESSED_PLAYER_CHANGED:
                    {
                        BE_ARRAY_TO_UINT16(tmp.u.addressed_player.player_id, p_data);
                        p_data += 2;
                        BE_ARRAY_TO_UINT16(tmp.u.addressed_player.uid_counter, p_data);
                        p_data += 2;
                    }
                    break;

                case EVENT_UIDS_CHANGED:
                    {
                        BE_ARRAY_TO_UINT16(tmp.u.uid_counter, p_data);
                    }
                    break;

                case EVENT_VOLUME_CHANGED:
                    {
                        tmp.u.volume = (*p_data++) & 0x7f;
                    }
                    break;

                default:
                    break;
                }

                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_NOTIF_CHANGED, &tmp);
            }
            else
            {
                T_RSP_REG_NOTIFICATION tmp;

                tmp.state = AVRCP_RSP_STATE_FAIL;
                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_REG_NOTIFICATION, &tmp);
            }
        }
        break;

    case PDU_ID_LIST_APP_SETTING_ATTRS:
        {
            T_AVRCP_RSP_LIST_APP_SETTING_ATTRS tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.num_of_attr = *p_data++;

                if (tmp.num_of_attr > MAX_APP_SETTING_ATTR_NUM)
                {
                    tmp.num_of_attr = MAX_APP_SETTING_ATTR_NUM;
                }
                for (int i = 0; i < tmp.num_of_attr; i++)
                {
                    tmp.attr_id[i] = *p_data++;
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_LIST_APP_SETTING_ATTRS, &tmp);
        }
        break;

    case PDU_ID_LIST_APP_SETTING_VALUES:
        {
            T_AVRCP_RSP_LIST_APP_SETTING_VALUES tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.num_of_value = *p_data++;

                if (tmp.num_of_value > MAX_APP_SETTING_VALUE_NUM)
                {
                    tmp.num_of_value = MAX_APP_SETTING_VALUE_NUM;
                }
                for (int i = 0; i < tmp.num_of_value; i++)
                {
                    tmp.value[i] = *p_data++;
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_LIST_APP_SETTING_VALUES, &tmp);
        }
        break;

    case PDU_ID_GET_CURRENT_APP_SETTING_VALUE:
        {
            T_AVRCP_RSP_GET_APP_SETTING_VALUE tmp;

            if (response == AVRCP_IMPLEMENTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.num_of_attr = *p_data++;

                if (tmp.num_of_attr > MAX_APP_SETTING_ATTR_NUM)
                {
                    tmp.num_of_attr = MAX_APP_SETTING_ATTR_NUM;
                }
                for (int i = 0; i < tmp.num_of_attr; i++)
                {
                    tmp.app_setting[i].attr = *p_data++;
                    tmp.app_setting[i].value = *p_data++;
                }
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_GET_CUR_APP_SETTING_VALUE, &tmp);
        }
        break;

    case PDU_ID_SET_ADDRESSED_PLAYER:
        {
            T_AVRCP_RSP_SET_ADDRESSED_PLAYER tmp;

            if (response == AVRCP_ACCEPTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_SET_ADDRESSED_PLAYER, &tmp);
        }
        break;

    case PDU_ID_PLAY_ITEM:
        {
            T_AVRCP_RSP_PLAY_ITEM tmp;

            if (response == AVRCP_ACCEPTED)
            {
                tmp.state = AVRCP_RSP_STATE_SUCCESS;
                tmp.status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
            }
            else
            {
                tmp.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_PLAY_ITEM, &tmp);
        }
        break;

    case PDU_ID_SET_ABSOLUTE_VOLUME:
        {
            T_AVRCP_RSP_SET_ABS_VOL rsp_abs_vol;

            if (response == AVRCP_ACCEPTED)
            {
                rsp_abs_vol.state = AVRCP_RSP_STATE_SUCCESS;
                rsp_abs_vol.volume = (*p_data++) & 0x7f;
            }
            else
            {
                rsp_abs_vol.state = AVRCP_RSP_STATE_FAIL;
            }

            p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_SET_ABS_VOL, &rsp_abs_vol);
        }
        break;

    case PDU_ID_SET_APP_SETTING_VALUE:
        break;

    case PDU_ID_REQUEST_CONTINUE_RSP: //only when TG response err
        break;

    case PDU_ID_ABORT_CONTINUE_RSP:
        break;

    default:
        break;
    }

    if (packet_type == AVRCP_PKT_TYPE_END) //normal clean
    {
        os_mem_free(p_link->ctrl_chann.recombine.p_buf);
        memset(&p_link->ctrl_chann.recombine, 0, sizeof(p_link->ctrl_chann.recombine));
    }

    return;
}

void avrcp_handle_vendor_rsp(T_AVRCP_LINK *p_link,
                             uint8_t      *p_oprand,
                             uint16_t      length,
                             uint8_t       response)
{
    uint8_t *p_data = p_oprand;
    uint32_t company_id = (p_data[0] << 16) | (p_data[1] << 8) | p_data[2];

    p_data += AVRCP_HDR_LENGTH;
    length -= AVRCP_HDR_LENGTH;

    if (company_id == COMPANY_BT_SIG)
    {
        avrcp_handle_vendor_BT_SIG_rsp(p_link, p_data, length, response);
    }
    else
    {
        T_AVRCP_VENDOR_RSP vendor_rsp;

        vendor_rsp.response = response;
        vendor_rsp.company_id = company_id;
        vendor_rsp.p_rsp = p_data;
        vendor_rsp.rsp_len = length;
        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_VENDOR_RSP, &vendor_rsp);
    }
}

void avrcp_handle_rsp(T_AVRCP_LINK *p_link,
                      uint8_t      *p_data,
                      uint16_t      length)
{
    uint8_t response = *p_data++ & 0x0f;
    p_data++;
//  subUnitType = *p_data & 0xF8; //unused
//  subUnitID = *p_data & 0x07; //unused
    uint8_t op_code = *p_data++;

    if (length >= AVCTP_NON_FRAG_HDR_LENGTH)
    {
        switch (op_code)
        {
        case OPCODE_UNIT_INFO:
            {
                T_RSP_UNIT_INFO tmp;

                tmp.state = AVRCP_RSP_STATE_FAIL;
                if (response == AVRCP_IMPLEMENTED)
                {
                    if (p_data[0] == 0x07) //const byte 0x07
                    {
                        tmp.state = AVRCP_RSP_STATE_SUCCESS;
                        tmp.sub_unit_type = p_data[1] >> 3;
                        tmp.sub_unit_id = p_data[1] & 0x07;
                        tmp.company_id = (p_data[2] << 16) | (p_data[3] << 8) | p_data[4];
                    }
                }

                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_UNIT_INFO, &tmp);
            }
            break;

        case OPCODE_PASS_THROUGH:
            {
                T_RSP_PASSTHROUGH tmp;

                if (response == AVRCP_ACCEPTED)
                {
                    uint8_t op_id = p_data[0] & 0x7f;
                    uint8_t state_flag = p_data[0] >> 7;

                    tmp.state = AVRCP_RSP_STATE_SUCCESS;
                    tmp.key = (T_AVRCP_KEY)op_id;
                    tmp.pressed = (state_flag == 0) ? true : false;
                }
                else
                {
                    tmp.state = AVRCP_RSP_STATE_FAIL;
                }

                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_PASSTHROUGH, &tmp);
            }
            break;

        case OPCODE_VENDOR_DEPENDENT:
            avrcp_handle_vendor_rsp(p_link, p_data, length - AVCTP_NON_FRAG_HDR_LENGTH, response);
            break;

        //  case OPCODE_SUB_UNIT_INFO: //useless temply
        //      {
        //          uint8_t *opRand = pL2cDataInd->p_buf + pL2cDataInd->dataOffset;
        //
        //          uint8_t subUnitTyte, subUnitId;
        //          int company_id = 0;
        //          if (response == AVRCP_IMPLEMENTED)
        //          {
        //              if (opRand[0] == 0x07) //page = 0 | no extension (7)
        //              {
        //                  int i;
        //                  for (i = 1; i < 5 && opRand[i] != 0xff; i++)
        //                  {
        //                      subUnitTyte = opRand[1] >> 3;
        //                      subUnitId = opRand[1] & 0x07;
        //                  }
        //              } else
        //              {
        //                  printf("Note: OPCODE_SUB_UNIT_INFO rsp extended\r\n");
        //              }
        //          } else
        //          {
        //              printf("Note: avrcp rsp[%x]\r\n", response);
        //          }
        //      }
        //      break;

        default:
            break;
        }
    }

    return;
}

void avrcp_handle_vendor_BT_SIG_cmd(T_AVRCP_LINK *p_link,
                                    uint8_t      *p_pdu,
                                    uint16_t      length,
                                    uint8_t       ctype,
                                    uint8_t       transact)
{
    uint8_t *p_data = p_pdu;
    uint8_t pdu_id = *p_data++;
    uint8_t packet_type = *p_data++ & 0x3;
    uint16_t para_length;

    BE_ARRAY_TO_UINT16(para_length, p_data);

    p_data += 2;

    if (packet_type != AVRCP_PKT_TYPE_NON_FRAGMENTED)
    {
        PROFILE_PRINT_ERROR0("avrcp rx cmd pdu dropped: fragmented pdu");
        return;
    }

    PROFILE_PRINT_TRACE3("avrcp_handle_vendor_BT_SIG_cmd: ctype 0x%02x, packet_type 0x%02x, pdu_id 0x%02x",
                         ctype, packet_type, pdu_id);
    switch (ctype)
    {
    case AVRCP_CONTROL:
        {
            switch (pdu_id)
            {
            case PDU_ID_SET_ABSOLUTE_VOLUME:
                if (p_avrcp->tg_features & AVRCP_CATEGORY_2)
                {
                    if (para_length == 1)
                    {
                        uint8_t volume = 0x7f & *p_data; // 1bit RFA

                        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_ABS_VOL, &volume);
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_ACCEPTED,
                                                     pdu_id, &volume, 1);
                    }
                    else
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {AVRCP_RSP_STATUS_PARAMETER_CONTENT_ERROR}, 1);
                    }
                }
                else
                {
                    avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                 pdu_id, p_data, para_length);
                }
                break;

#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
            case PDU_ID_SET_ADDRESSED_PLAYER:
                if (p_avrcp->tg_features & AVRCP_CATEGORY_1 ||
                    p_avrcp->tg_features & AVRCP_CATEGORY_3)
                {
                    if (para_length == 2)
                    {
                        uint16_t player_id;

                        BE_ARRAY_TO_UINT16(player_id, p_data);

                        p_link->ctrl_chann.set_addressed_player_pending_transact = transact;
                        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_SET_ADDRESSED_PLAYER, &player_id);
                    }
                    else
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {AVRCP_RSP_STATUS_PARAMETER_CONTENT_ERROR}, 1);
                    }
                }
                else
                {
                    avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                 pdu_id, p_data, para_length);
                }
                break;

            case PDU_ID_REQUEST_CONTINUE_RSP:
                {
                    uint8_t target_pdu_id = *p_data;

                    if (p_link->ctrl_chann.fragment.pdu_id == target_pdu_id)
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_IMPLEMENTED, target_pdu_id,
                                                     p_link->ctrl_chann.fragment.p_buf + p_link->ctrl_chann.fragment.read_index,
                                                     p_link->ctrl_chann.fragment.total_length - p_link->ctrl_chann.fragment.read_index);
                    }
                    else
                    {
                        if (p_link->ctrl_chann.fragment.p_buf != NULL)
                        {
                            os_mem_free(p_link->ctrl_chann.fragment.p_buf);
                            memset(&p_link->ctrl_chann.fragment, 0, sizeof(p_link->ctrl_chann.fragment));
                        }

                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                    }
                }
                break;

            case PDU_ID_ABORT_CONTINUE_RSP:
                {
                    uint8_t target_pdu_id = *p_data;

                    if (p_link->ctrl_chann.fragment.pdu_id == target_pdu_id)
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_ACCEPTED, pdu_id,
                                                     NULL, 0);
                    }
                    else
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                    }

                    if (p_link->ctrl_chann.fragment.p_buf != NULL)
                    {
                        os_mem_free(p_link->ctrl_chann.fragment.p_buf);
                        memset(&p_link->ctrl_chann.fragment, 0, sizeof(p_link->ctrl_chann.fragment));
                    }
                }
                break;
#endif

            default:
                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                             pdu_id, p_data, para_length);
                PROFILE_PRINT_WARN1("avrcp rx unsupported vendor cmd,CONTROL pdu id[%x]", pdu_id);
                break;
            }
        }
        break;

    case AVRCP_STATUS:
        {
            switch (pdu_id)
            {
            case PDU_ID_GET_CAPABILITIES:
                {
                    uint8_t capability_id = *p_data;

                    switch (capability_id)
                    {
                    case CAPABILITY_ID_COMPANY_ID:
                        {
                            if (p_avrcp->local_company_id == COMPANY_BT_SIG) //no specific vendor
                            {
                                uint8_t para[] =
                                {
                                    capability_id,
                                    1 /* company count*/,
                                    (uint8_t)(COMPANY_BT_SIG >> 16),
                                    (uint8_t)(COMPANY_BT_SIG >> 8),
                                    (uint8_t)COMPANY_BT_SIG
                                };

                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_IMPLEMENTED,
                                                             pdu_id, para, sizeof(para));
                            }
                            else
                            {
                                uint32_t company_id = p_avrcp->local_company_id;
                                uint8_t para[] =
                                {
                                    capability_id,
                                    2 /* company count*/,
                                    (uint8_t)(COMPANY_BT_SIG >> 16),
                                    (uint8_t)(COMPANY_BT_SIG >> 8),
                                    (uint8_t)COMPANY_BT_SIG,
                                    (uint8_t)(company_id >> 16),
                                    (uint8_t)(company_id >> 8),
                                    (uint8_t)company_id
                                };

                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_IMPLEMENTED,
                                                             pdu_id, para, sizeof(para));
                            }
                        }
                        break;

                    case CAPABILITY_ID_EVENTS_SUPPORTED:
                        {
                            uint8_t para[10];
                            uint8_t count = 0;
                            uint8_t *p = para;

                            *p++ = capability_id;
                            *p++ = 0;
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                *p++ = EVENT_PLAYBACK_STATUS_CHANGED;
                                *p++ = EVENT_TRACK_CHANGED;
                                *p++ = EVENT_AVAILABLE_PLAYERS_CHANGED;
                                *p++ = EVENT_ADDRESSED_PLAYER_CHANGED;
                                count += 4;
                            }
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_2)
                            {
                                *p++ = EVENT_VOLUME_CHANGED;
                                count++;
                            }
                            para[1] = count;

                            avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_IMPLEMENTED,
                                                         pdu_id, para, p - para);
                        }
                        break;

                    default:
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                                                     &(uint8_t)
                        {
                            AVRCP_RSP_STATUS_INVALID_PARAMETER
                        }, 1);
                        break;
                    }
                }
                break;

#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
            case PDU_ID_GET_ELEMENT_ATTRS:
                {
                    if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                    {
                        T_AVRCP_REQ_GET_ELEMENT_ATTR get_element_attrs;
                        int i;

                        p_data += 8;  /* ignore identifier*/
                        get_element_attrs.attr_num = *p_data++;
                        for (i = 0; i < get_element_attrs.attr_num; i++)
                        {
                            BE_ARRAY_TO_UINT32(get_element_attrs.attr_id[i], p_data);
                            p_data += 4;
                        }

                        p_link->ctrl_chann.get_element_attr_pending_transact = transact;
                        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_GET_ELEMENT_ATTRS,
                                       &get_element_attrs);
                    }
                    else
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                     pdu_id, p_data, para_length);
                    }
                }
                break;

            case PDU_ID_GET_PLAY_STATUS:
                {
                    if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                    {
                        p_link->ctrl_chann.get_play_status_pending_transact = transact;
                        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_GET_PLAY_STATUS, NULL);
                    }
                    else
                    {
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                     pdu_id, p_data, para_length);
                    }
                }
                break;
#endif

            default:
                {
                    avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                    &(uint8_t) {AVRCP_RSP_STATUS_INVALID_COMMAND}, 1);
                    PROFILE_PRINT_WARN1("avrcp rx unsupported vendor cmd,STATUS pdu id[%x]", pdu_id);
                }
                break;
            }
        }
        break;

    case AVRCP_NOTIFY:
        {
            switch (pdu_id)
            {
            case PDU_ID_REGISTER_NOTIFICATION:
                {
                    uint8_t event_id = *p_data;

                    switch (event_id)
                    {
                    case EVENT_VOLUME_CHANGED:
                        {
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_2)
                            {
                                p_link->ctrl_chann.vol_change_pending_transact = transact;
                                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_REG_VOL_CHANGE, NULL);
                            }
                            else
                            {
                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                             pdu_id, p_data, para_length);
                            }
                        }
                        break;

                    case EVENT_PLAYBACK_STATUS_CHANGED:
                        {
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                p_link->ctrl_chann.play_status_change_pending_transact = transact;
                                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_REG_PLAY_STATUS_CHANGE, NULL);
                            }
                            else
                            {
                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                             pdu_id, p_data, para_length);
                            }
                        }
                        break;

#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
                    case EVENT_TRACK_CHANGED:
                        {
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                p_link->ctrl_chann.track_change_pending_transact = transact;
                                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_REG_TRACK_CHANGE, NULL);
                            }
                            else
                            {
                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                             pdu_id, p_data, para_length);
                            }
                        }
                        break;

                    case EVENT_AVAILABLE_PLAYERS_CHANGED:
                        {
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_INTERIM,
                                                             pdu_id, &event_id, 1);
                            }
                            else
                            {
                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                             pdu_id, p_data, para_length);
                            }
                        }
                        break;

                    case EVENT_ADDRESSED_PLAYER_CHANGED:
                        {
                            if (p_avrcp->tg_features & AVRCP_CATEGORY_1)
                            {
                                p_link->ctrl_chann.addressed_player_change_pending_transact = transact;
                                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_REG_ADDRESSED_PLAYER_CHANGE, NULL);
                            }
                            else
                            {
                                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                                             pdu_id, p_data, para_length);
                            }
                        }
                        break;
#endif

                    default:
                        avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_REJECTED, pdu_id,
                        &(uint8_t) {AVRCP_RSP_STATUS_INVALID_PARAMETER}, 1);
                        break;
                    }
                }
                break;

            default:
                avrcp_send_vendor_BT_SIG_rsp(p_link, transact, AVRCP_NOT_IMPLEMENTED,
                                             pdu_id, p_data, para_length);
                PROFILE_PRINT_WARN1("avrcp rx unsupported vendor cmd,NOTIFY pdu id[%x]", pdu_id);
                break;
            }
        }
        break;

    default:
        break;
    }
}

void avrcp_handle_vendor_cmd(T_AVRCP_LINK *p_link,
                             uint8_t      *p_oprand,
                             uint16_t      length,
                             uint8_t       ctype,
                             uint8_t       transact)
{
    uint8_t *p_data = p_oprand;
    uint32_t company_id = (p_data[0] << 16) | (p_data[1] << 8) | p_data[2];

    p_data += AVRCP_HDR_LENGTH;
    length -= AVRCP_HDR_LENGTH;

    if (company_id == COMPANY_BT_SIG)
    {
        avrcp_handle_vendor_BT_SIG_cmd(p_link, p_data, length, ctype, transact);
    }
    else
    {
        T_AVRCP_VENDOR_CMD vendor_cmd;

        p_link->ctrl_chann.vendor_cmd_transact = transact;

        vendor_cmd.ctype = ctype;
        vendor_cmd.company_id = company_id;
        vendor_cmd.p_cmd = p_data;
        vendor_cmd.cmd_len = length;
        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_VENDOR_CMD_IND, &vendor_cmd);
    }
}

void avrcp_handle_cmd(T_AVRCP_LINK *p_link,
                      uint8_t      *p_data,
                      uint16_t      length,
                      uint8_t       transact)
{
    uint8_t ctype = *p_data++ & 0x0f;
    uint8_t sub_unit_type_and_id = *p_data++;
    uint8_t op_code = *p_data++;
    uint8_t response = 0;

    if (length >= AVCTP_NON_FRAG_HDR_LENGTH)
    {
        if ((sub_unit_type_and_id == 0xff) ||
            (sub_unit_type_and_id == ((AVRCP_SUBUNIT_TYPE_PANEL << 3) | AVRCP_SUBUNIT_ID)))
        {
            switch (ctype)
            {
            case AVRCP_CONTROL:
                {
                    switch (op_code)
                    {
                    case OPCODE_PASS_THROUGH:
                        {
                            uint8_t state = *p_data >> 7;
                            uint8_t op_id = *p_data & 0x7f;

                            switch (op_id)
                            {
                            case AVRCP_KEY_POWER:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_POWER, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_VOL_UP:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_VOL_UP, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_VOL_DOWN:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_VOL_DOWN, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_MUTE:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_MUTE, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_PLAY:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_PLAY, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_STOP:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_STOP, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_PAUSE:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_PAUSE, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_REWIND:
                                if (state == PASSTHROUGH_STATE_PRESSED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_REWIND_START, NULL);
                                }
                                else
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_REWIND_STOP, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_FAST_FORWARD:
                                if (state == PASSTHROUGH_STATE_PRESSED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_FAST_FORWARD_START, NULL);
                                }
                                else
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_FAST_FORWARD_STOP, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_FORWARD:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_FORWARD, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            case AVRCP_KEY_BACKWARD:
                                if (state == PASSTHROUGH_STATE_RELEASED)
                                {
                                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_BACKWARD, NULL);
                                }
                                response = AVRCP_ACCEPTED;
                                break;

                            default:
                                response = AVRCP_NOT_IMPLEMENTED;
                                break;
                            }
                        }
                        break;

                    case OPCODE_VENDOR_DEPENDENT:
                        avrcp_handle_vendor_cmd(p_link, p_data, length - AVCTP_NON_FRAG_HDR_LENGTH,
                                                AVRCP_CONTROL, transact);
                        break;

                    default:
                        response = AVRCP_NOT_IMPLEMENTED;
                        break;
                    }
                }
                break;

            case AVRCP_STATUS:
                {
                    switch (op_code)
                    {
                    case OPCODE_UNIT_INFO:
                        {
                            uint32_t company_id = p_avrcp->local_company_id;

                            p_data -= AVCTP_NON_FRAG_HDR_LENGTH;
                            p_data[0] = (*p_data & 0xf0) | (AVRCP_IMPLEMENTED & 0x0f);
                            p_data[3] = 0x07; //const byte 0x07
                            p_data[4] = AVRCP_SUBUNIT_TYPE_PANEL << 3 | 0;
                            p_data[5] = (uint8_t)(company_id >> 16);
                            p_data[6] = (uint8_t)(company_id >> 8);
                            p_data[7] = (uint8_t)company_id;

                            avctp_send_data(p_link->bd_addr, p_data, length, transact, AVCTP_MSG_TYPE_RSP);
                        }
                        break;

                    case OPCODE_SUB_UNIT_INFO:
                        p_data -= AVCTP_NON_FRAG_HDR_LENGTH;
                        p_data[0] = (*p_data & 0xf0) | (AVRCP_IMPLEMENTED & 0x0f);
                        p_data[3] = 0x07; //page = 0 | no extension (7)
                        p_data[4] = AVRCP_SUBUNIT_TYPE_PANEL << 3 | 0; //sub unit type | max subunit id
                        p_data[5] = 0xff;
                        p_data[6] = 0xff;
                        p_data[7] = 0xff;

                        avctp_send_data(p_link->bd_addr, p_data, length, transact, AVCTP_MSG_TYPE_RSP);
                        break;

                    case OPCODE_VENDOR_DEPENDENT:
                        avrcp_handle_vendor_cmd(p_link, p_data, length - AVCTP_NON_FRAG_HDR_LENGTH,
                                                AVRCP_STATUS, transact);
                        break;

                    default:
                        response = AVRCP_NOT_IMPLEMENTED;
                    }
                }
                break;

            case AVRCP_NOTIFY:
                {
                    switch (op_code)
                    {
                    case OPCODE_VENDOR_DEPENDENT:
                        avrcp_handle_vendor_cmd(p_link, p_data, length - AVCTP_NON_FRAG_HDR_LENGTH,
                                                AVRCP_NOTIFY, transact);
                        break;

                    default:
                        response = AVRCP_NOT_IMPLEMENTED;
                        break;
                    }
                }
                break;

            case AVRCP_SPECIFIC_INQUIRY:
                response = AVRCP_NOT_IMPLEMENTED;
                break;

            case AVRCP_GENERAL_INQUIRY:
                response = AVRCP_NOT_IMPLEMENTED;
                break;

            default:
                response = AVRCP_NOT_IMPLEMENTED;
                break;
            }
        }
        else
        {
            response = AVRCP_NOT_IMPLEMENTED;
        }
    }
    else
    {
        response = AVRCP_NOT_IMPLEMENTED;
    }

    if (response != 0)
    {
        p_data -= AVCTP_NON_FRAG_HDR_LENGTH;
        *p_data = (*p_data & 0xf0) | (response & 0x0f);
        avctp_send_data(p_link->bd_addr, p_data, length, transact, AVCTP_MSG_TYPE_RSP);
    }
}

void avrcp_handle_data_ind(uint16_t  cid,
                           uint8_t   transact,
                           uint8_t   cr_type,
                           uint8_t  *p_data,
                           uint16_t  length)
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_cid(cid);

    if (p_link != NULL)
    {
        if (cr_type == AVCTP_MSG_TYPE_CMD)
        {
            avrcp_handle_cmd(p_link, p_data, length, transact);
        }
        else //AVCTP_MSG_TYPE_RSP
        {
            //ds cmd is blocked (to avoid repeated rsp with the same label, eg.AVRCP_CHANGED)
            if (p_link->ctrl_chann.cmd_credits == 0)
                //to avoid repeated rsp not for last sent cmd, eg.AVRCP_CHANGED)
                //&& (transact == p_link->transact_label))
            {
                p_link->ctrl_chann.cmd_credits = 1;
                sys_timer_stop(p_link->ctrl_chann.timer_handle);
            }

            avrcp_handle_rsp(p_link, p_data, length);
            avrcp_cmd_process(p_link);
        }
    }
}

bool avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                      uint8_t volume)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_VOLUME_CHANGED,
        volume
    };

    if (volume > 0x7f)
    {
        ret = 1;
        goto fail_invalid_volume;
    }

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_2) == 0)
    {
        ret = 2;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 3;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.vol_change_pending_transact == 0xff)
    {
        ret = 4;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.vol_change_pending_transact, AVRCP_INTERIM,
                                 PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
fail_invalid_volume:
    PROFILE_PRINT_ERROR1("avrcp_volume_change_register_rsp: failed %d", -ret);
    return false;
}

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
void avrcp_browsing_handle_data_ind(uint16_t  cid,
                                    uint8_t   transact,
                                    uint8_t   cr_type,
                                    uint8_t  *p_data,
                                    uint16_t  length)
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_cid(cid);
    uint8_t       pdu_id = *p_data++;
    int           i;
    //uint16_t para_length;

    //BE_ARRAY_TO_UINT16(para_length, p_data);
    p_data += 2;

    if (p_link != NULL)
    {
        if (cr_type == AVCTP_MSG_TYPE_RSP)
        {
            p_link->browsing_chann.cmd_credits = 1;

            switch (pdu_id)
            {
            case PDU_ID_SET_BROWSED_PLAYER:
                {
                    T_AVRCP_RSP_SET_BROWSED_PLAYER tmp;

                    tmp.status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
                    BE_ARRAY_TO_UINT16(tmp.uid_counter, p_data);
                    p_data += 2;
                    BE_ARRAY_TO_UINT32(tmp.num_of_items, p_data);
                    p_data += 4;
                    BE_ARRAY_TO_UINT16(tmp.character_set_id, p_data);
                    p_data += 2;
                    tmp.folder_depth = *p_data++;

                    if (tmp.folder_depth > MAX_FOLDER_DEPTH)
                    {
                        tmp.folder_depth = MAX_FOLDER_DEPTH;
                    }
                    for (i = 0; i < tmp.folder_depth; i++)
                    {
                        BE_ARRAY_TO_UINT16(tmp.folder[i].name_length, p_data);
                        p_data += 2;
                        tmp.folder[i].p_name = p_data++;
                    }

                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_SET_BROWSED_PLAYER, &tmp);
                }
                break;

            case PDU_ID_GET_FOLDER_ITEMS:
                {
                    uint8_t j;
                    T_RSP_GET_FOLDER_ITEMS tmp;
                    uint8_t *p_buf = NULL;

                    tmp.status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
                    BE_ARRAY_TO_UINT16(tmp.uid_counter, p_data);
                    p_data += 2;
                    BE_ARRAY_TO_UINT16(tmp.num_of_items, p_data);
                    p_data += 2;

                    if (tmp.num_of_items > 0)
                    {
                        tmp.item_type = *p_data;

                        switch (tmp.item_type)
                        {
                        case AVRCP_ITEM_TYPE_MEDIA_PLAYER:
                            {
                                T_MEDIA_PLAYER_ITEM *p_item;

                                p_buf = os_mem_alloc2(sizeof(T_MEDIA_PLAYER_ITEM) * tmp.num_of_items);
                                if (p_buf == NULL)
                                {
                                    PROTOCOL_PRINT_WARN0("AVRCP_ITEM_TYPE_MEDIA_PLAYER: get memory fail");
                                    return;
                                }

                                for (i = 0; i < tmp.num_of_items; i++)
                                {
                                    p_item = (T_MEDIA_PLAYER_ITEM *)p_buf + i;

                                    p_item->item_type = *p_data++;
                                    BE_ARRAY_TO_UINT16(p_item->item_length, p_data);
                                    p_data += 2;
                                    BE_ARRAY_TO_UINT16(p_item->player_id, p_data);
                                    p_data += 2;
                                    p_item->major_type = *p_data++;
                                    BE_ARRAY_TO_UINT32(p_item->sub_type, p_data);
                                    p_data += 4;
                                    p_item->play_status = *p_data++;
                                    memcpy(&p_item->feature_bitmask[0], p_data, 16);
                                    p_data += 16;
                                    BE_ARRAY_TO_UINT16(p_item->character_set_id, p_data);
                                    p_data += 2;
                                    BE_ARRAY_TO_UINT16(p_item->display_name_length, p_data);
                                    p_data += 2;
                                    p_item->p_display_name = p_data;
                                    p_data += p_item->display_name_length;
                                }

                                tmp.u.p_media_player_items = (T_MEDIA_PLAYER_ITEM *)p_buf;
                            }
                            break;

                        case AVRCP_ITEM_TYPE_FOLDER:
                            {
                                T_FOLDER_ITEM *p_item;

                                p_buf = os_mem_alloc2(sizeof(T_FOLDER_ITEM) * tmp.num_of_items);
                                if (p_buf == NULL)
                                {
                                    PROTOCOL_PRINT_WARN0("AVRCP_ITEM_TYPE_FOLDER: get memory fail");
                                    return;
                                }

                                for (i = 0; i < tmp.num_of_items; i++)
                                {
                                    p_item = (T_FOLDER_ITEM *)p_buf + i;

                                    p_item->item_type = *p_data++;
                                    BE_ARRAY_TO_UINT16(p_item->item_length, p_data);
                                    p_data += 2;
                                    BE_ARRAY_TO_UINT64(p_item->folder_uid, p_data);
                                    p_data += 8;
                                    p_item->folder_type = *p_data++;
                                    p_item->is_playable = *p_data++;
                                    BE_ARRAY_TO_UINT16(p_item->character_set_id, p_data);
                                    p_data += 2;
                                    BE_ARRAY_TO_UINT16(p_item->display_name_length, p_data);
                                    p_data += 2;
                                    p_item->p_display_name = p_data;
                                    p_data += p_item->display_name_length;
                                }

                                tmp.u.p_folder_items = (T_FOLDER_ITEM *)p_buf;
                            }
                            break;

                        case AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
                            {
                                T_MEDIA_ELEMENT_ITEM *p_item;

                                p_buf = os_mem_alloc2(sizeof(T_MEDIA_ELEMENT_ITEM) * tmp.num_of_items);
                                if (p_buf == NULL)
                                {
                                    PROTOCOL_PRINT_WARN0("AVRCP_ITEM_TYPE_MEDIA_ELEMENT: get memory fail");
                                    return;
                                }

                                for (i = 0; i < tmp.num_of_items; i++)
                                {
                                    p_item = (T_MEDIA_ELEMENT_ITEM *)p_buf + i;

                                    p_item->item_type = *p_data++;
                                    BE_ARRAY_TO_UINT16(p_item->item_length, p_data);
                                    p_data += 2;
                                    BE_ARRAY_TO_UINT64(p_item->media_element_uid, p_data);
                                    p_data += 8;
                                    p_item->media_type = *p_data++;
                                    BE_ARRAY_TO_UINT16(p_item->character_set_id, p_data);
                                    p_data += 2;
                                    BE_ARRAY_TO_UINT16(p_item->display_name_length, p_data);
                                    p_data += 2;
                                    p_item->p_display_name = p_data;
                                    p_data += p_item->display_name_length;
                                    p_item->num_of_attr = *p_data++;
                                    for (j = 0; j < p_item->num_of_attr; j++)
                                    {
                                        BE_ARRAY_TO_UINT32(p_item->attr[j].attribute_id, p_data);
                                        p_data += 4;
                                        BE_ARRAY_TO_UINT16(p_item->attr[j].character_set_id, p_data);
                                        p_data += 2;
                                        BE_ARRAY_TO_UINT16(p_item->attr[j].length, p_data);
                                        p_data += 2;
                                        p_item->attr[j].p_buf = p_data;
                                        p_data += p_item->attr[j].length;
                                    }
                                }
                                tmp.u.p_media_element_items = (T_MEDIA_ELEMENT_ITEM *)p_buf;
                            }
                            break;

                        default:
                            return;
                        }
                    }

                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_GET_FOLDER_ITEMS, &tmp);
                    if (p_buf != NULL)
                    {
                        os_mem_free(p_buf);
                    }
                }
                break;

            case PDU_ID_CHANGE_PATH:
                {
                    T_AVRCP_RSP_CHANGE_PATH tmp;

                    tmp.status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
                    BE_ARRAY_TO_UINT32(tmp.num_of_items, p_data);
                    p_data += 4;

                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_CHANGE_PATH, &tmp);
                }
                break;

            case PDU_ID_GET_ITEM_ATTRS:
                {
                    T_AVRCP_RSP_GET_ITEM_ATTRS *tmp = os_mem_alloc2(sizeof(T_AVRCP_RSP_GET_ITEM_ATTRS));
                    if (tmp == NULL)
                    {
                        PROTOCOL_PRINT_WARN0("avrcp_browsing_handle_data_ind: get memory fail for element attrs");
                        return;
                    }

                    tmp->status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
                    tmp->num_of_attr = *p_data++;
                    if (tmp->num_of_attr > MAX_ELEMENT_ATTR_NUM)
                    {
                        tmp->num_of_attr = MAX_ELEMENT_ATTR_NUM;
                    }
                    for (i = 0; i < tmp->num_of_attr; i++)
                    {
                        BE_ARRAY_TO_UINT32(tmp->attr[i].attribute_id, p_data);
                        p_data += 4;
                        BE_ARRAY_TO_UINT16(tmp->attr[i].character_set_id, p_data);
                        p_data += 2;
                        BE_ARRAY_TO_UINT16(tmp->attr[i].length, p_data);
                        p_data += 2;
                        tmp->attr[i].p_buf = p_data;
                        p_data += tmp->attr[i].length;
                    }

                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_GET_ITEM_ATTRS, tmp);
                    os_mem_free(tmp);
                }
                break;

            case PDU_ID_SEARCH:
                {
                    T_AVRCP_RSP_SEARCH tmp;

                    tmp.status_code = (T_AVRCP_RSP_ERROR_STATUS) * p_data++;
                    BE_ARRAY_TO_UINT16(tmp.uid_counter, p_data);
                    p_data += 2;
                    BE_ARRAY_TO_UINT32(tmp.num_of_items, p_data);
                    p_data += 4;

                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_RSP_SEARCH, &tmp);
                }
                break;

            default:
                break;
            }

            avrcp_browsing_cmd_process(p_link);
        }
#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
        else if (cr_type == AVCTP_MSG_TYPE_CMD)
        {
            p_link->browsing_chann.pending_transact = transact;

            switch (pdu_id)
            {
            case PDU_ID_GET_FOLDER_ITEMS:
                {
                    T_CMD_GET_FOLDER_ITEMS cmd_get_folder_items;

                    cmd_get_folder_items.scope_id = *p_data++;
                    BE_ARRAY_TO_UINT32(cmd_get_folder_items.start_item, p_data);
                    p_data += 4;
                    BE_ARRAY_TO_UINT32(cmd_get_folder_items.end_item, p_data);
                    p_data += 4;
                    cmd_get_folder_items.attr_count = *p_data++;
                    for (i = 0; i < cmd_get_folder_items.attr_count; i++)
                    {
                        BE_ARRAY_TO_UINT32(cmd_get_folder_items.attr_id[i], p_data);
                        p_data += 4;
                    }

                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_GET_FOLDER_ITEMS,
                                   &cmd_get_folder_items);
                }
                break;

            case PDU_ID_GET_TOTAL_NUM_OF_ITEMS:
                {
                    uint8_t scope_id;

                    scope_id = *p_data++;
                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CMD_GET_TOTAL_NUM_OF_ITEMS,
                                   &scope_id);
                }
                break;

            default:
                {
                    uint8_t para[] =
                    {
                        PDU_ID_GENERAL_REJECT,
                        0x00,
                        0x01,
                        AVRCP_RSP_STATUS_INVALID_COMMAND
                    };

                    avrcp_browsing_send_generic_rsp(p_link, para, sizeof(para));
                }
                break;
            }
        }
#endif
    }
}
#endif

bool avrcp_notify_volume_change(uint8_t bd_addr[6],
                                uint8_t volume)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_VOLUME_CHANGED,
        volume
    };

    if (volume > 0x7f)
    {
        ret = 1;
        goto fail_invalid_volume;
    }

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_2) == 0)
    {
        ret = 2;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 3;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.vol_change_pending_transact == 0xff)
    {
        ret = 4;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.vol_change_pending_transact, AVRCP_CHANGED,
                                 PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));
    p_link->ctrl_chann.vol_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
fail_invalid_volume:
    PROFILE_PRINT_ERROR1("avrcp_notify_volume_change: failed %d", -ret);
    return false;
}

bool avrcp_play_status_change_register_rsp(uint8_t bd_addr[6],
                                           uint8_t play_status)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_PLAYBACK_STATUS_CHANGED,
        play_status
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.play_status_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.play_status_change_pending_transact,
                                 AVRCP_INTERIM, PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_play_status_change_register_rsp: failed %d", -ret);
    return false;
}

bool avrcp_notify_play_status_change(uint8_t bd_addr[6],
                                     uint8_t play_status)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_PLAYBACK_STATUS_CHANGED,
        play_status
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.play_status_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.play_status_change_pending_transact,
                                 AVRCP_CHANGED, PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));
    p_link->ctrl_chann.play_status_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_notify_play_status_change: failed %d", -ret);
    return false;
}

bool avrcp_connect_req(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;
    PROFILE_PRINT_TRACE1("avrcp_connect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL) //No avrcp connection request from remote device
    {
        p_link = avrcp_alloc_link(bd_addr);
        if (p_link != NULL)
        {
            p_link->ctrl_chann.state = AVRCP_STATE_CONNECTING;
            return avctp_connect_req(bd_addr);
        }
    }
    return false;
}

void avrcp_handle_conn_ind(T_MPA_L2C_CONN_IND *ind)
{
    T_AVRCP_LINK *p_link;
    PROFILE_PRINT_TRACE2("avrcp_handle_conn_ind: bd_addr [%s], cid 0x%04x",
                         TRACE_BDADDR(ind->bd_addr), ind->cid);

    p_link = avrcp_find_link_by_addr(ind->bd_addr);
    if (p_link == NULL)
    {
        p_link = avrcp_alloc_link(ind->bd_addr);
        if (p_link != NULL)
        {
            p_link->ctrl_chann.state = AVRCP_STATE_CONNECTING;
            p_link->ctrl_chann.cid = ind->cid;
            p_link->ctrl_chann.cmd_credits = 0;
            p_avrcp->cback(ind->bd_addr, AVRCP_MSG_CONN_IND, NULL);
        }
        else
        {
            avctp_connect_cfm(ind->bd_addr, false);
        }
    }
    else
    {
        avctp_connect_cfm(ind->bd_addr, false);
    }
}

bool avrcp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept)
{
    T_AVRCP_LINK *p_link;
    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (accept == false)
        {
            avrcp_free_link(p_link);
        }
        return avctp_connect_cfm(bd_addr, accept);
    }
    return avctp_connect_cfm(bd_addr, false);
}

void avrcp_handle_conn_rsp(T_MPA_L2C_CONN_RSP *ind)
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_addr(ind->bd_addr);

    if (p_link != NULL)
    {
        p_link->ctrl_chann.cid = ind->cid;
        p_link->ctrl_chann.cmd_credits = 0;
    }
}

void avrcp_handle_conn_fail(uint16_t cid,
                            uint16_t cause)
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_cid(cid);

    if (p_link != NULL)
    {
        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_CONN_FAIL, &cause);
        avrcp_free_link(p_link);
    }
}

void avrcp_handle_conn_cmpl_ind(T_MPA_L2C_CONN_CMPL_INFO *ind)
{
    T_AVRCP_LINK *p_link;

    PROFILE_PRINT_TRACE2("avrcp_handle_conn_cmpl_ind: bd_addr [%s], remote_mtu %d",
                         TRACE_BDADDR(ind->bd_addr), ind->remote_mtu);

    p_link = avrcp_find_link_by_addr(ind->bd_addr);
    if (p_link != NULL)
    {
        os_queue_init(&p_link->ctrl_chann.cmd_queue);
        os_queue_init(&p_link->browsing_chann.cmd_queue);

        p_link->ctrl_chann.state = AVRCP_STATE_CONNECTED;
        p_link->ctrl_chann.cmd_credits = 1;
        p_link->ctrl_chann.transact_label = 0;
        p_link->ctrl_chann.remote_mtu = ind->remote_mtu;
        p_link->ctrl_chann.vol_change_pending_transact = 0xff;
        p_link->ctrl_chann.play_status_change_pending_transact = 0xff;
        p_link->ctrl_chann.track_change_pending_transact = 0xff;
        p_link->ctrl_chann.addressed_player_change_pending_transact = 0xff;
        p_link->ctrl_chann.get_element_attr_pending_transact = 0xff;
        p_link->ctrl_chann.get_play_status_pending_transact = 0xff;
        p_link->ctrl_chann.set_addressed_player_pending_transact = 0xff;
        p_link->ctrl_chann.vendor_cmd_transact = 0xff;
        memset(&p_link->ctrl_chann.fragment, 0, sizeof(p_link->ctrl_chann.fragment));

        p_avrcp->cback(ind->bd_addr, AVRCP_MSG_CONN_CMPL, NULL);
    }
}

bool avrcp_disconnect_req(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;
    bool ret = false;
    PROFILE_PRINT_TRACE1("avrcp_disconnect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        ret = avctp_disconnect_req(p_link->bd_addr);
        if (ret)
        {
            p_link->ctrl_chann.state = AVRCP_STATE_DISCONNECTING;
        }
    }

    return ret;
}

void avrcp_handle_disconn_ind(uint16_t cid,
                              uint16_t cause)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_cid(cid);
    if (p_link != NULL)
    {
        T_AVRCP_DISCONN_INFO info;
        T_AVRCP_CMD *p_data;
        T_AVRCP_CMD *p_next;

        PROFILE_PRINT_TRACE2("avrcp_handle_disconn_ind: bd_addr [%s], cid 0x%04x",
                             TRACE_BDADDR(p_link->bd_addr), cid);

        p_data = (T_AVRCP_CMD *)p_link->ctrl_chann.cmd_queue.p_first;
        while (p_data)
        {
            p_next = (T_AVRCP_CMD *)p_data->p_next;

            os_queue_delete(&p_link->ctrl_chann.cmd_queue, p_data);
            os_mem_free(p_data);

            p_data = p_next;
        }

        info.cause = cause;
        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_DISCONN, &info);
        avrcp_free_link(p_link);
    }
}

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
bool avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;
    PROFILE_PRINT_TRACE1("avrcp_browsing_connect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_link->browsing_chann.state = AVRCP_STATE_CONNECTING;
        return avctp_browsing_connect_req(bd_addr);
    }

    return false;
}

void avrcp_handle_browsing_conn_ind(T_MPA_L2C_CONN_IND *ind)
{
    T_AVRCP_LINK *p_link;
    PROFILE_PRINT_TRACE2("avrcp_handle_browsing_conn_ind: bd_addr [%s], cid 0x%04x",
                         TRACE_BDADDR(ind->bd_addr), ind->cid);

    p_link = avrcp_find_link_by_addr(ind->bd_addr);
    if (p_link == NULL)
    {
        avctp_browsing_connect_cfm(ind->bd_addr, false);
    }
    else
    {
        p_link->browsing_chann.state = AVRCP_STATE_CONNECTING;
        p_link->browsing_chann.cid = ind->cid;
        p_link->browsing_chann.cmd_credits = 0;
        p_avrcp->cback(ind->bd_addr, AVRCP_MSG_BROWSING_CONN_IND, NULL);
    }
}

bool avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (accept == false)
        {
            p_link->browsing_chann.state = AVRCP_STATE_DISCONNECTED;
        }
        return avctp_browsing_connect_cfm(bd_addr, accept);
    }
    return avctp_browsing_connect_cfm(bd_addr, false);
}

void avrcp_handle_browsing_conn_rsp(T_MPA_L2C_CONN_RSP *ind)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(ind->bd_addr);
    if (p_link != NULL)
    {
        p_link->browsing_chann.cid = ind->cid;
        p_link->browsing_chann.cmd_credits = 0;
    }
}

void avrcp_handle_browsing_conn_cmpl_ind(T_MPA_L2C_CONN_CMPL_INFO *ind)
{
    T_AVRCP_LINK *p_link;
    PROFILE_PRINT_TRACE2("avrcp_handle_browsing_conn_cmpl_ind: bd_addr [%s], remoteMTU %d",
                         TRACE_BDADDR(ind->bd_addr), ind->remote_mtu);

    p_link = avrcp_find_link_by_addr(ind->bd_addr);
    if (p_link != NULL)
    {
        p_link->browsing_chann.state = AVRCP_STATE_CONNECTED;
        p_link->browsing_chann.cmd_credits = 1;
        p_link->browsing_chann.transact_label = 0;
        p_link->browsing_chann.pending_transact = 0xff;

        p_avrcp->cback(ind->bd_addr, AVRCP_MSG_BROWSING_CONN_CMPL, NULL);

        avrcp_browsing_cmd_process(p_link);
    }
}

bool avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;
    bool ret = false;
    PROFILE_PRINT_TRACE1("avrcp_browsing_disconnect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        ret = avctp_browsing_disconnect_req(p_link->bd_addr);
        if (ret)
        {
            p_link->browsing_chann.state = AVRCP_STATE_DISCONNECTING;
        }
    }

    return ret;
}

void avrcp_handle_browsing_disconn_ind(uint16_t cid,
                                       uint16_t cause)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_cid(cid);
    if (p_link != NULL)
    {
        T_AVRCP_DISCONN_INFO info;
        T_AVRCP_BROWSING_CMD *p_data;
        T_AVRCP_BROWSING_CMD *p_next;

        PROFILE_PRINT_TRACE2("avrcp_handle_browsing_disconn_ind: bd_addr [%s], cid 0x%04x",
                             TRACE_BDADDR(p_link->bd_addr), cid);

        p_data = (T_AVRCP_BROWSING_CMD *)p_link->browsing_chann.cmd_queue.p_first;
        while (p_data)
        {
            p_next = (T_AVRCP_BROWSING_CMD *)p_data->p_next;

            os_queue_delete(&p_link->browsing_chann.cmd_queue, p_data);
            if (p_data->pdu_id == PDU_ID_SEARCH)
            {
                if (p_data->cmd_param.search.p_search_str != NULL)
                {
                    os_mem_free(p_data->cmd_param.search.p_search_str);
                }
            }
            os_mem_free(p_data);

            p_data = p_next;
        }

        info.cause = cause;
        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_BROWSING_DISCONN, &info);
        memset(&(p_link->browsing_chann), 0, sizeof(T_AVRCP_BROWSING_CHANN));
    }
}
#else
bool avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}
#endif

bool avrcp_set_absolute_volume(uint8_t bd_addr[6],
                               uint8_t absolute_volume)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_avrcp->ct_features & AVRCP_CATEGORY_2)
        {
            uint8_t volume;

            volume = absolute_volume & 0x7f;
            return avrcp_cmd_enqueue(p_link, PDU_ID_SET_ABSOLUTE_VOLUME, &volume);
        }
    }

    return false;
}

bool avrcp_get_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    T_AVRCP_LINK *p_link = avrcp_find_link_by_addr(bd_addr);

    if (p_link == NULL)
    {
        return false;
    }

    p_info->l2c_cid = p_link->ctrl_chann.cid;
    p_info->cmd_credits = p_link->ctrl_chann.cmd_credits;
    p_info->transact_label = p_link->ctrl_chann.transact_label;
    p_info->vol_change_pending_transact = p_link->ctrl_chann.vol_change_pending_transact;
    p_info->vendor_cmd_transact = p_link->ctrl_chann.vendor_cmd_transact;
    p_info->state = p_link->ctrl_chann.state;

    return avctp_get_roleswap_info(bd_addr, p_info);
}

bool avrcp_set_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = avrcp_alloc_link(bd_addr);
    }

    if (p_link == NULL)
    {
        return false;
    }

    p_link->ctrl_chann.cid = p_info->l2c_cid;
    p_link->ctrl_chann.cmd_credits = p_info->cmd_credits;
    p_link->ctrl_chann.transact_label = p_info->transact_label;
    p_link->ctrl_chann.vol_change_pending_transact = p_info->vol_change_pending_transact;
    p_link->ctrl_chann.vendor_cmd_transact = p_info->vendor_cmd_transact;
    p_link->ctrl_chann.state = (T_AVRCP_STATE)p_info->state;

    if (p_link->ctrl_chann.cmd_credits == 0)
    {
        sys_timer_start(p_link->ctrl_chann.timer_handle);
    }

    return avctp_set_roleswap_info(bd_addr, p_info);
}

bool avrcp_del_roleswap_info(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    avrcp_free_link(p_link);

    return avctp_del_roleswap_info(bd_addr);
}

void avrcp_callback(uint16_t     cid,
                    T_AVCTP_MSG  msg_type,
                    void        *msg_buf)
{
    switch (msg_type)
    {
    case AVCTP_MSG_CONN_IND:
        avrcp_handle_conn_ind((T_MPA_L2C_CONN_IND *)msg_buf);
        break;

    case AVCTP_MSG_CONN_RSP:
        avrcp_handle_conn_rsp((T_MPA_L2C_CONN_RSP *)msg_buf);
        break;

    case AVCTP_MSG_CONN_FAIL:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            avrcp_handle_conn_fail(cid, cause);
        }
        break;

    case AVCTP_MSG_CONN_CMPL_IND:
        avrcp_handle_conn_cmpl_ind((T_MPA_L2C_CONN_CMPL_INFO *)msg_buf);
        break;

    case AVCTP_MSG_DATA_IND:
        {
            uint8_t *p_data = ((T_AVCTP_DATA_IND *)msg_buf)->p_data;
            uint16_t length = ((T_AVCTP_DATA_IND *)msg_buf)->length;
            uint8_t crtype = ((T_AVCTP_DATA_IND *)msg_buf)->crtype;
            uint8_t transact_label = ((T_AVCTP_DATA_IND *)msg_buf)->transact_label;
            avrcp_handle_data_ind(cid, transact_label, crtype, p_data, length);
        }
        break;

    case AVCTP_MSG_DISCONN_IND:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            avrcp_handle_disconn_ind(cid, cause);
        }
        break;

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
    case AVCTP_MSG_BROWSING_CONN_IND:
        avrcp_handle_browsing_conn_ind((T_MPA_L2C_CONN_IND *)msg_buf);
        break;

    case AVCTP_MSG_BROWSING_CONN_RSP:
        avrcp_handle_browsing_conn_rsp((T_MPA_L2C_CONN_RSP *)msg_buf);
        break;

    case AVCTP_MSG_BROWSING_CONN_CMPL_IND:
        avrcp_handle_browsing_conn_cmpl_ind((T_MPA_L2C_CONN_CMPL_INFO *)msg_buf);
        break;

    case AVCTP_MSG_BROWSING_DATA_IND:
        {
            uint8_t *p_data = ((T_AVCTP_DATA_IND *)msg_buf)->p_data;
            uint16_t length = ((T_AVCTP_DATA_IND *)msg_buf)->length;
            uint8_t crtype = ((T_AVCTP_DATA_IND *)msg_buf)->crtype;
            uint8_t transact_label = ((T_AVCTP_DATA_IND *)msg_buf)->transact_label;
            avrcp_browsing_handle_data_ind(cid, transact_label, crtype, p_data, length);
        }
        break;

    case AVCTP_MSG_BROWSING_DISCONN_IND:
        {
            uint16_t cause = *((uint16_t *)msg_buf);
            avrcp_handle_browsing_disconn_ind(cid, cause);
        }
        break;
#endif

    default:
        break;
    }
}

bool avrcp_set_supported_features(uint8_t ct_features,
                                  uint8_t tg_features)
{
    if (p_avrcp != NULL)
    {
        p_avrcp->ct_features = ct_features;
        p_avrcp->tg_features = tg_features;

        return true;
    }

    return false;
}

bool avrcp_init(uint8_t       link_num,
                uint32_t      company_id,
                P_AVRCP_CBACK cback)
{
    int32_t ret = 0;

    if ((cback == NULL) || ((company_id & 0xFF000000) != 0)) //24-bits company id
    {
        ret = 1;
        goto fail_invalid_para;
    }

    p_avrcp = os_mem_zalloc2(sizeof(T_AVRCP));
    if (p_avrcp == NULL)
    {
        ret = 2;
        goto fail_alloc_avrcp;
    }

    p_avrcp->link_num = link_num;

    p_avrcp->link_list = os_mem_zalloc2(p_avrcp->link_num * sizeof(T_AVRCP_LINK));
    if (p_avrcp->link_list == NULL)
    {
        ret = 3;
        goto fail_alloc_link;
    }

    p_avrcp->wait_rsp_tout         = 3000;    // uint is ms
    p_avrcp->local_company_id      = company_id;
    p_avrcp->cback                 = cback;
    p_avrcp->ct_features           = AVRCP_CATEGORY_1;
    p_avrcp->tg_features           = AVRCP_CATEGORY_2;

    /**Protocol init*/
    if (avctp_init(p_avrcp->link_num, avrcp_callback) == false)
    {
        ret = 4;
        goto fail_init_avctp;
    }

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
    if (obex_init() == false)
    {
        ret = 5;
        goto fail_init_obex;
    }
#endif

    return true;

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
fail_init_obex:
#endif
fail_init_avctp:
    os_mem_free(p_avrcp->link_list);
fail_alloc_link:
    os_mem_free(p_avrcp);
    p_avrcp = NULL;
fail_alloc_avrcp:
fail_invalid_para:
    PROFILE_PRINT_ERROR1("avrcp_init: failed %d", -ret);
    return false;
}

#if (CONFIG_REALTEK_BTM_AVRCP_TG_SUPPORT == 1)
bool avrcp_get_play_status_rsp(uint8_t  bd_addr[6],
                               uint32_t song_length,
                               uint32_t song_pos,
                               uint8_t  play_status)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        (uint8_t)(song_length >> 24),
        (uint8_t)(song_length >> 18),
        (uint8_t)(song_length >> 8),
        (uint8_t)song_length,
        (uint8_t)(song_pos >> 24),
        (uint8_t)(song_pos >> 18),
        (uint8_t)(song_pos >> 8),
        (uint8_t)song_pos,
        play_status
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.get_play_status_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.get_play_status_pending_transact,
                                 AVRCP_IMPLEMENTED, PDU_ID_GET_PLAY_STATUS, para, sizeof(para));
    p_link->ctrl_chann.get_play_status_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_get_play_status_rsp: failed %d", -ret);
    return false;
}

bool avrcp_get_element_attr_rsp(uint8_t         bd_addr[6],
                                uint8_t         attr_num,
                                T_ELEMENT_ATTR *p_attr)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t i;
    uint8_t *p_data;
    uint8_t *p;
    uint16_t data_len;

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.get_element_attr_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    data_len = 1;
    for (i = 0; i < attr_num; i++)
    {
        data_len += 8;
        data_len += p_attr[i].length;
    }

    p_data = os_mem_alloc2(data_len);
    if (p_data == NULL)
    {
        ret = 4;
        goto fail_alloc_mem;
    }

    if (p_link->ctrl_chann.fragment.p_buf != NULL)
    {
        ret = 5;
        goto fail_last_rsp_not_finish;
    }

    p = p_data;

    BE_UINT8_TO_STREAM(p, attr_num);
    for (i = 0; i < attr_num; i++)
    {
        BE_UINT32_TO_STREAM(p, p_attr[i].attribute_id);
        BE_UINT16_TO_STREAM(p, p_attr[i].character_set_id);
        BE_UINT16_TO_STREAM(p, p_attr[i].length);
        memcpy(p, p_attr[i].p_buf, p_attr[i].length);
        p += p_attr[i].length;
    }

    p_link->ctrl_chann.fragment.p_buf = p_data;
    p_link->ctrl_chann.fragment.read_index = 0;
    p_link->ctrl_chann.fragment.total_length = data_len;

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.get_element_attr_pending_transact,
                                 AVRCP_IMPLEMENTED, PDU_ID_GET_ELEMENT_ATTRS, p_data, data_len);
    p_link->ctrl_chann.get_element_attr_pending_transact = 0xff;

    return true;

fail_last_rsp_not_finish:
    os_mem_free(p_data);
fail_alloc_mem:
fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_get_element_attr_rsp: failed %d", -ret);
    return false;
}

bool avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                     uint64_t track_id)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_TRACK_CHANGED,
        (uint8_t)(track_id >> 56),
        (uint8_t)(track_id >> 48),
        (uint8_t)(track_id >> 40),
        (uint8_t)(track_id >> 32),
        (uint8_t)(track_id >> 24),
        (uint8_t)(track_id >> 18),
        (uint8_t)(track_id >> 8),
        (uint8_t)track_id
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.track_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.track_change_pending_transact,
                                 AVRCP_INTERIM, PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_track_change_register_rsp: failed %d", -ret);
    return false;
}

bool avrcp_notify_track_change(uint8_t  bd_addr[6],
                               uint64_t track_id)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_TRACK_CHANGED,
        (uint8_t)(track_id >> 56),
        (uint8_t)(track_id >> 48),
        (uint8_t)(track_id >> 40),
        (uint8_t)(track_id >> 32),
        (uint8_t)(track_id >> 24),
        (uint8_t)(track_id >> 18),
        (uint8_t)(track_id >> 8),
        (uint8_t)track_id
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.track_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.track_change_pending_transact,
                                 AVRCP_CHANGED, PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));
    p_link->ctrl_chann.track_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_notify_track_change: failed %d", -ret);
    return false;
}

bool avrcp_set_addressed_player_rsp(uint8_t bd_addr[6],
                                    uint8_t status)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.set_addressed_player_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    if (status == AVRCP_RSP_STATUS_SUCCESS)
    {
        avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.set_addressed_player_pending_transact,
                                     AVRCP_ACCEPTED, PDU_ID_SET_ADDRESSED_PLAYER, &status, 1);
    }
    else
    {
        avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.set_addressed_player_pending_transact,
                                     AVRCP_REJECTED, PDU_ID_SET_ADDRESSED_PLAYER, &status, 1);
    }
    p_link->ctrl_chann.set_addressed_player_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_set_addressed_player_rsp: failed %d", -ret);
    return false;
}

bool avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                uint16_t player_id,
                                                uint16_t uid_counter)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_ADDRESSED_PLAYER_CHANGED,
        (uint8_t)(player_id >> 8),
        (uint8_t)player_id,
        (uint8_t)(uid_counter >> 8),
        (uint8_t)uid_counter
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0 &&
        (p_avrcp->tg_features & AVRCP_CATEGORY_3) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.addressed_player_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.addressed_player_change_pending_transact,
                                 AVRCP_INTERIM, PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_addressed_player_change_register_rsp: failed %d", -ret);
    return false;
}

bool avrcp_notify_addressed_player_change(uint8_t bd_addr[6], uint16_t player_id,
                                          uint16_t uid_counter)
{
    T_AVRCP_LINK *p_link;
    int32_t ret = 0;
    uint8_t para[] =
    {
        EVENT_ADDRESSED_PLAYER_CHANGED,
        (uint8_t)(player_id >> 8),
        (uint8_t)player_id,
        (uint8_t)(uid_counter >> 8),
        (uint8_t)uid_counter
    };

    if ((p_avrcp->tg_features & AVRCP_CATEGORY_1) == 0 &&
        (p_avrcp->tg_features & AVRCP_CATEGORY_3) == 0)
    {
        ret = 1;
        goto fail_invalid_category;
    }

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_invalid_addr;
    }

    if (p_link->ctrl_chann.addressed_player_change_pending_transact == 0xff)
    {
        ret = 3;
        goto fail_no_pending_transact;
    }

    avrcp_send_vendor_BT_SIG_rsp(p_link, p_link->ctrl_chann.addressed_player_change_pending_transact,
                                 AVRCP_CHANGED, PDU_ID_REGISTER_NOTIFICATION, para, sizeof(para));
    p_link->ctrl_chann.addressed_player_change_pending_transact = 0xff;

    return true;

fail_no_pending_transact:
fail_invalid_addr:
fail_invalid_category:
    PROFILE_PRINT_ERROR1("avrcp_notify_addressed_player_change: failed %d", -ret);
    return false;
}

bool avrcp_browsing_get_folder_items_rsp(uint8_t   bd_addr[6],
                                         uint8_t   status,
                                         uint16_t  uid_counter,
                                         uint16_t  num_of_items,
                                         void     *p_items)
{
    T_AVRCP_LINK *p_link;
    int32_t  ret;
    uint8_t *p_param;
    uint8_t *p;
    uint8_t  i;
    uint8_t  item_type;
    uint16_t param_len = 5;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_find_link;
    }

    if ((num_of_items == 0) || (p_items == NULL))
    {
        ret = 2;
        goto fail_check_param;
    }

    item_type = *(uint8_t *)p_items;
    switch (item_type)
    {
    case AVRCP_ITEM_TYPE_MEDIA_PLAYER:
        {
            T_MEDIA_PLAYER_ITEM *p_item = (T_MEDIA_PLAYER_ITEM *)p_items;

            for (i = 0; i < num_of_items; i++)
            {
                param_len += 3;
                param_len += ((p_item + i)->item_length);
            }
        }
        break;

    case AVRCP_ITEM_TYPE_FOLDER:
        {
            T_FOLDER_ITEM *p_item = (T_FOLDER_ITEM *)p_items;

            for (i = 0; i < num_of_items; i++)
            {
                param_len += 3;
                param_len += ((p_item + i)->item_length);
            }
        }
        break;

    case AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
        {
            T_MEDIA_ELEMENT_ITEM *p_item = (T_MEDIA_ELEMENT_ITEM *)p_items;

            for (i = 0; i < num_of_items; i++)
            {
                param_len += 3;
                param_len += ((p_item + i)->item_length);
            }
        }
        break;

    default:
        {
            ret = 3;
            goto fail_invalid_item_type;
        }
    }

    p_param = os_mem_alloc2(param_len + 3);
    if (p_param == NULL)
    {
        ret = 4;
        goto fail_alloc_mem;
    }

    p = p_param;

    BE_UINT8_TO_STREAM(p, PDU_ID_GET_FOLDER_ITEMS);
    BE_UINT16_TO_STREAM(p, param_len);
    BE_UINT8_TO_STREAM(p, status);
    BE_UINT16_TO_STREAM(p, uid_counter);
    BE_UINT16_TO_STREAM(p, num_of_items);
    switch (item_type)
    {
    case AVRCP_ITEM_TYPE_MEDIA_PLAYER:
        {
            T_MEDIA_PLAYER_ITEM *p_item = (T_MEDIA_PLAYER_ITEM *)p_items;

            for (i = 0; i < num_of_items; i++)
            {
                BE_UINT8_TO_STREAM(p, p_item->item_type);
                BE_UINT16_TO_STREAM(p, p_item->item_length);
                BE_UINT16_TO_STREAM(p, p_item->player_id);
                BE_UINT8_TO_STREAM(p, p_item->major_type);
                BE_UINT32_TO_STREAM(p, p_item->sub_type);
                BE_UINT8_TO_STREAM(p, p_item->play_status);
                ARRAY_TO_STREAM(p, p_item->feature_bitmask, 16);
                BE_UINT16_TO_STREAM(p, p_item->character_set_id);
                BE_UINT16_TO_STREAM(p, p_item->display_name_length);
                ARRAY_TO_STREAM(p, p_item->p_display_name, p_item->display_name_length);
            }
        }
        break;

    case AVRCP_ITEM_TYPE_FOLDER:
        {
            T_FOLDER_ITEM *p_item = (T_FOLDER_ITEM *)p_items;

            for (i = 0; i < num_of_items; i++)
            {
                BE_UINT8_TO_STREAM(p, p_item->item_type);
                BE_UINT16_TO_STREAM(p, p_item->item_length);
                BE_UINT64_TO_STREAM(p, p_item->folder_uid);
                BE_UINT8_TO_STREAM(p, p_item->folder_type);
                BE_UINT8_TO_STREAM(p, p_item->is_playable);
                BE_UINT16_TO_STREAM(p, p_item->character_set_id);
                BE_UINT16_TO_STREAM(p, p_item->display_name_length);
                ARRAY_TO_STREAM(p, p_item->p_display_name, p_item->display_name_length);
            }
        }
        break;

    case AVRCP_ITEM_TYPE_MEDIA_ELEMENT:
        {
            T_MEDIA_ELEMENT_ITEM *p_item = (T_MEDIA_ELEMENT_ITEM *)p_items;
            uint8_t j;

            for (i = 0; i < num_of_items; i++)
            {
                BE_UINT8_TO_STREAM(p, p_item->item_type);
                BE_UINT16_TO_STREAM(p, p_item->item_length);
                BE_UINT64_TO_STREAM(p, p_item->media_element_uid);
                BE_UINT8_TO_STREAM(p, p_item->media_type);
                BE_UINT16_TO_STREAM(p, p_item->character_set_id);
                BE_UINT16_TO_STREAM(p, p_item->display_name_length);
                ARRAY_TO_STREAM(p, p_item->p_display_name, p_item->display_name_length);
                BE_UINT8_TO_STREAM(p, p_item->num_of_attr);
                for (j = 0; j < p_item->num_of_attr; j++)
                {
                    BE_UINT32_TO_STREAM(p, p_item->attr[j].attribute_id);
                    BE_UINT16_TO_STREAM(p, p_item->attr[j].character_set_id);
                    BE_UINT16_TO_STREAM(p, p_item->attr[j].length);
                    ARRAY_TO_STREAM(p, p_item->attr[j].p_buf, p_item->attr[j].length);
                }
            }
        }
        break;

    default:
        break;
    }

    if (avrcp_browsing_send_generic_rsp(p_link, p_param, param_len + 3) == false)
    {
        ret = 5;
        goto fail_send_rsp;
    }

    os_mem_free(p_param);
    return true;

fail_send_rsp:
    os_mem_free(p_param);
fail_alloc_mem:
fail_invalid_item_type:
fail_check_param:
fail_find_link:
    PROFILE_PRINT_ERROR1("avrcp_browsing_get_folder_items_rsp: failed %d", -ret);
    return false;
}

bool avrcp_browsing_get_total_num_of_items_rsp(uint8_t  bd_addr[6],
                                               uint8_t  status,
                                               uint16_t uid_counter,
                                               uint32_t num_of_items)
{
    T_AVRCP_LINK *p_link;
    uint8_t para[] =
    {
        PDU_ID_GET_TOTAL_NUM_OF_ITEMS,
        0x00,
        0x07,
        status,
        (uint8_t)(uid_counter >> 8),
        (uint8_t)uid_counter,
        (uint8_t)(num_of_items >> 24),
        (uint8_t)(num_of_items >> 16),
        (uint8_t)(num_of_items >> 8),
        (uint8_t)num_of_items,
    };

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_browsing_send_generic_rsp(p_link, para, sizeof(para));
    }

    return false;
}
#endif

#if (CONFIG_REALTEK_BTM_AVRCP_BROWSING_SUPPORT == 1)
bool avrcp_list_app_setting_attrs(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_LIST_APP_SETTING_ATTRS, NULL);
    }

    return false;
}

bool avrcp_list_app_setting_values(uint8_t bd_addr[6],
                                   uint8_t attr_id)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_LIST_APP_SETTING_VALUES, &attr_id);
    }

    return false;
}

bool avrcp_get_current_app_setting_value(uint8_t  bd_addr[6],
                                         uint8_t  attr_num,
                                         uint8_t *p_attr)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_GET_APP_SETTING_VALUE tmp;
        uint8_t i;

        tmp.attr_num = attr_num;
        for (i = 0; i < attr_num; i++)
        {
            tmp.attr_id[i] = p_attr[i];
        }

        return avrcp_cmd_enqueue(p_link, PDU_ID_GET_CURRENT_APP_SETTING_VALUE, &tmp);
    }

    return false;
}

bool avrcp_set_app_setting_value(uint8_t              bd_addr[6],
                                 uint8_t              attr_num,
                                 T_AVRCP_APP_SETTING *p_app_setting)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_SET_APP_SETTING_VALUE tmp;
        uint8_t i;

        tmp.attr_num = attr_num;
        for (i = 0; i < attr_num; i++)
        {
            tmp.app_setting[i].attr = p_app_setting[i].attr;
            tmp.app_setting[i].value = p_app_setting[i].value;
        }

        return avrcp_cmd_enqueue(p_link, PDU_ID_SET_APP_SETTING_VALUE, &tmp);
    }

    return false;
}

bool avrcp_req_continuing_rsp(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_REQUEST_CONTINUE_RSP, &p_link->ctrl_chann.last_pdu_id);
    }

    return false;
}

bool avrcp_abort_continuing_rsp(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_ABORT_CONTINUE_RSP, &p_link->ctrl_chann.last_pdu_id);
    }

    return false;
}

bool avrcp_set_addressed_player(uint8_t  bd_addr[6],
                                uint16_t player_id)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_cmd_enqueue(p_link, PDU_ID_SET_ADDRESSED_PLAYER, &player_id);
    }

    return false;
}

bool avrcp_play_item(uint8_t          bd_addr[6],
                     T_AVRCP_SCOPE_ID scope_id,
                     uint64_t         uid,
                     uint16_t         uid_counter)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_PLAY_ITEM tmp;

        tmp.scope_id = scope_id;
        tmp.uid = uid;
        tmp.uid_counter = uid_counter;

        return avrcp_cmd_enqueue(p_link, PDU_ID_PLAY_ITEM, &tmp);
    }

    return false;
}

bool avrcp_navigate_group(uint8_t bd_addr[6],
                          bool    next,
                          bool    pressed)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        T_AVRCP_REQ_PASS_THROUGH tmp;

        tmp.key = (next ? AVRCP_PASS_THROUGH_NEXT_GROUP : AVRCP_PASS_THROUGH_PREVIOUS_GROUP);
        tmp.pressed = pressed;

        return avrcp_cmd_enqueue(p_link, PDU_ID_PASS_THROUGH, &tmp);
    }

    return false;
}

bool avrcp_browsing_get_folder_items(uint8_t                       bd_addr[6],
                                     T_AVRCP_REQ_GET_FOLDER_ITEMS *p_cmd_para)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_browsing_cmd_enqueue(p_link, PDU_ID_GET_FOLDER_ITEMS, p_cmd_para);
    }

    return false;
}

bool avrcp_browsing_get_item_attrs(uint8_t                     bd_addr[6],
                                   T_AVRCP_REQ_GET_ITEM_ATTRS *p_cmd_para)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_browsing_cmd_enqueue(p_link, PDU_ID_GET_ITEM_ATTRS, p_cmd_para);
    }

    return false;
}

bool avrcp_browsing_search(uint8_t             bd_addr[6],
                           T_AVRCP_REQ_SEARCH *p_cmd_para)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_browsing_cmd_enqueue(p_link, PDU_ID_SEARCH, p_cmd_para);
    }

    return false;
}

bool avrcp_browsing_set_browsed_player(uint8_t  bd_addr[6],
                                       uint16_t player_id)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_browsing_cmd_enqueue(p_link, PDU_ID_SET_BROWSED_PLAYER, &player_id);
    }

    return false;
}

bool avrcp_browsing_change_path(uint8_t                  bd_addr[6],
                                T_AVRCP_REQ_CHANGE_PATH *p_cmd_para)
{
    T_AVRCP_LINK *p_link;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return avrcp_browsing_cmd_enqueue(p_link, PDU_ID_CHANGE_PATH, p_cmd_para);
    }

    return false;
}
#endif

#if (CONFIG_REALTEK_BTM_AVRCP_COVER_ART_SUPPORT == 1)
void avrcp_cover_art_obex_cb(uint8_t  msg_type,
                             void    *p_msg)
{
    T_AVRCP_LINK *p_link;

    PROFILE_PRINT_TRACE1("avrcp_cover_art_obex_cb: msg_type=0x%x", msg_type);

    switch (msg_type)
    {
    case OBEX_CB_MSG_CONN_IND:
        break;

    case OBEX_CB_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *p_data = (T_OBEX_CONN_CMPL_DATA *)p_msg;

            p_link = avrcp_find_link_by_addr(p_data->bd_addr);
            if (p_link && (p_link->handle == p_data->handle))
            {
                p_link->cover_art_state = AVRCP_COVER_ART_STATE_CONNECTED;
                if (p_avrcp->cback)
                {
                    p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_COVER_ART_CONN_CMPL, NULL);
                }
            }
            else
            {
                obex_disconn_req(p_data->handle);
            }
        }
        break;

    case OBEX_CB_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *p_data = (T_OBEX_DISCONN_CMPL_DATA *)p_msg;

            p_link = avrcp_find_link_by_addr(p_data->bd_addr);
            if (p_link)
            {
                if (p_data->handle == p_link->handle)
                {
                    p_link->handle = NULL;
                    p_link->cover_art_state = AVRCP_COVER_ART_STATE_DISCONNECTED;
                    if (p_avrcp->cback)
                    {
                        T_AVRCP_DISCONN_INFO info;
                        info.cause = p_data->cause;
                        p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_COVER_ART_DISCONN, &info);
                    }
                }
            }
        }
        break;

    case OBEX_CB_MSG_GET_OBJECT:
        {
            T_OBEX_GET_OBJECT_CMPL_DATA *p_data = (T_OBEX_GET_OBJECT_CMPL_DATA *)p_msg;
            uint8_t *p_hdr_data;
            uint16_t hdr_data_len;
            bool ret;

            p_link = avrcp_find_link_by_addr(p_data->bd_addr);
            if (p_link == NULL || p_link->handle != p_data->handle || p_avrcp->cback == NULL)
            {
                break;
            }

            if (p_data->rsp_code != OBEX_RSP_SUCCESS && p_data->rsp_code != OBEX_RSP_CONTINUE)
            {
                p_link->cover_art_state = AVRCP_COVER_ART_STATE_CONNECTED;
                PROFILE_PRINT_ERROR1("OBEX_CB_MSG_GET_OBJECT: error rsp_code=0x%x", p_data->rsp_code);
                break;
            }

            if (p_link->cover_art_state == AVRCP_COVER_ART_STATE_GET_IMAGE ||
                p_link->cover_art_state == AVRCP_COVER_ART_STATE_GET_THUMBNAIL)
            {
                T_AVRCP_COVER_ART_MSG_DATA data = {0};

                ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_BODY,
                                           &p_hdr_data, &hdr_data_len);
                if (ret == false)
                {
                    ret = obex_find_hdr_in_pkt(p_data->p_data, p_data->data_len, OBEX_HI_END_BODY, &p_hdr_data,
                                               &hdr_data_len);
                    if (ret)
                    {
                        data.data_end = true;
                    }
                }
                else
                {
                    data.data_end = false;
                }

                if (ret)
                {
                    data.p_data = p_hdr_data;
                    data.data_len = hdr_data_len;
                }

                //workaround for iphone cover art which has no END_OF_BODY header
                if (p_data->rsp_code == OBEX_RSP_SUCCESS)
                {
                    data.data_end = true;
                }

                if (data.data_end)
                {
                    p_link->cover_art_state = AVRCP_COVER_ART_STATE_CONNECTED;
                }

                p_avrcp->cback(p_link->bd_addr, AVRCP_MSG_COVER_ART_DATA_IND, (void *)&data);
            }
        }
        break;

    default:
        break;
    }
}

bool avrcp_cover_art_conn_over_l2c(uint8_t  bd_addr[6],
                                   uint16_t l2c_psm)
{
    T_AVRCP_LINK *p_link;
    uint8_t *p_param;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (p_link->cover_art_state == AVRCP_COVER_ART_STATE_DISCONNECTED)
    {
        p_link->cover_art_state = AVRCP_COVER_ART_STATE_CONNECTING;
        PROFILE_PRINT_TRACE1("avrcp_cover_art_conn_over_l2c: bd_addr[%s]", TRACE_BDADDR(bd_addr));

        p_param = (uint8_t *)avrcp_cover_art_target;
        if (obex_conn_req_over_l2c(bd_addr, p_param, l2c_psm, avrcp_cover_art_obex_cb, &p_link->handle))
        {
            return true;
        }
        else
        {
            p_link->cover_art_state = AVRCP_COVER_ART_STATE_DISCONNECTED;
            return false;
        }
    }

    return false;
}

bool avrcp_cover_art_disconnect(uint8_t bd_addr[6])
{
    T_AVRCP_LINK *p_link;
    bool ret = false;

    PROFILE_PRINT_TRACE1("avrcp_disconnect_req: bd_addr[%s]", TRACE_BDADDR(bd_addr));

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link != NULL && p_link->handle != NULL)
    {
        ret = obex_disconn_req(p_link->handle);
        if (ret)
        {
            p_link->cover_art_state = AVRCP_COVER_ART_STATE_DISCONNECTING;
        }
    }

    return ret;
}

bool avrcp_cover_art_get_image(uint8_t  bd_addr[6],
                               uint8_t *img_handle)
{
    T_AVRCP_LINK *p_link;
    uint16_t pkt_len;
    uint8_t *p_data;
    uint8_t *p;
    bool ret = false;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL || p_link->handle == NULL)
    {
        return false;
    }

    if (p_link->cover_art_state == AVRCP_COVER_ART_STATE_GET_IMAGE)
    {
        ret = obex_get_object_continue(p_link->handle);
    }
    else if (p_link->cover_art_state == AVRCP_COVER_ART_STATE_CONNECTED)
    {
        pkt_len = AVRCP_COVER_ART_TYPE_LEN + (3 + AVRCP_IMAGE_HANDLE_LEN) + 3;

        p_data = os_mem_alloc2(pkt_len);
        if (p_data == NULL)
        {
            return false;
        }
        p = p_data;

        ARRAY_TO_STREAM(p, image_type, AVRCP_COVER_ART_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, AVRCP_IMAGE_HANDLE_HEADER_ID);
        BE_UINT16_TO_STREAM(p, 3 + AVRCP_IMAGE_HANDLE_LEN);
        ARRAY_TO_STREAM(p, img_handle, AVRCP_IMAGE_HANDLE_LEN);
        BE_UINT8_TO_STREAM(p, AVRCP_IMAGE_DESCRIPTION_HEADER_ID);
        BE_UINT16_TO_STREAM(p, 3);

        p_link->cover_art_state = AVRCP_COVER_ART_STATE_GET_IMAGE;

        ret = obex_get_object(p_link->handle, p_data, p - p_data);
        os_mem_free(p_data);
    }

    return ret;
}

bool avrcp_cover_art_get_linked_thumbnail(uint8_t  bd_addr[6],
                                          uint8_t *img_handle)
{
    T_AVRCP_LINK *p_link;
    uint16_t pkt_len;
    uint8_t *p_data;
    uint8_t *p;
    bool ret = false;

    p_link = avrcp_find_link_by_addr(bd_addr);
    if (p_link == NULL || p_link->handle == NULL)
    {
        return false;
    }

    if (p_link->cover_art_state == AVRCP_COVER_ART_STATE_GET_THUMBNAIL)
    {
        ret = obex_get_object_continue(p_link->handle);
    }
    else if (p_link->cover_art_state == AVRCP_COVER_ART_STATE_CONNECTED)
    {
        pkt_len = AVRCP_COVER_ART_TYPE_LEN + (3 + AVRCP_IMAGE_HANDLE_LEN);

        p_data = os_mem_alloc2(pkt_len);
        if (p_data == NULL)
        {
            return false;
        }
        p = p_data;

        ARRAY_TO_STREAM(p, image_thumbnail_type, AVRCP_COVER_ART_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, AVRCP_IMAGE_HANDLE_HEADER_ID);
        BE_UINT16_TO_STREAM(p, 3 + AVRCP_IMAGE_HANDLE_LEN);
        ARRAY_TO_STREAM(p, img_handle, AVRCP_IMAGE_HANDLE_LEN);

        p_link->cover_art_state = AVRCP_COVER_ART_STATE_GET_THUMBNAIL;

        ret = obex_get_object(p_link->handle, p_data, p - p_data);
        os_mem_free(p_data);
    }

    return ret;
}
#else
bool avrcp_cover_art_disconnect(uint8_t bd_addr[6])
{
    return false;
}
#endif
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "avrcp.h"

bool avrcp_init(uint8_t       link_num,
                uint32_t      company_id,
                P_AVRCP_CBACK cback)
{
    return false;
}

bool avrcp_set_supported_features(uint8_t ct_features,
                                  uint8_t tg_features);

bool avrcp_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_connect_cfm(uint8_t bd_addr[6],
                       bool    accept)
{
    return false;
}

bool avrcp_send_unit_info(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_send_pass_through(uint8_t     bd_addr[6],
                             T_AVRCP_KEY key,
                             bool        pressed)
{
    return false;
}

bool avrcp_get_capability(uint8_t bd_addr[6],
                          uint8_t capability_id)
{
    return false;
}

bool avrcp_get_play_status(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_get_play_status_rsp(uint8_t  bd_addr[6],
                               uint32_t song_length,
                               uint32_t song_pos,
                               uint8_t  play_status)
{
    return false;
}

bool avrcp_get_element_attr(uint8_t  bd_addr[6],
                            uint8_t  attr_num,
                            uint8_t *p_attr)
{
    return false;
}

bool avrcp_get_element_attr_rsp(uint8_t         bd_addr[6],
                                uint8_t         attr_num,
                                T_ELEMENT_ATTR *p_attr)
{
    return false;
}

bool avrcp_register_notification(uint8_t bd_addr[6],
                                 uint8_t event_id)
{
    return false;
}

bool avrcp_volume_change_register_rsp(uint8_t bd_addr[6],
                                      uint8_t volume)
{
    return false;
}

bool avrcp_notify_volume_change(uint8_t bd_addr[6],
                                uint8_t volume)
{
    return false;
}

bool avrcp_play_status_change_register_rsp(uint8_t bd_addr[6],
                                           uint8_t play_status)
{
    return false;
}

bool avrcp_notify_play_status_change(uint8_t bd_addr[6],
                                     uint8_t play_status)
{
    return false;
}

bool avrcp_track_change_register_rsp(uint8_t  bd_addr[6],
                                     uint64_t track_id)
{
    return false;
}

bool avrcp_notify_track_change(uint8_t  bd_addr[6],
                               uint64_t track_id)
{
    return false;
}

bool avrcp_get_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    return false;
}

bool avrcp_set_roleswap_info(uint8_t                bd_addr[6],
                             T_ROLESWAP_AVRCP_INFO *p_info)
{
    return false;
}

bool avrcp_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_list_app_setting_attrs(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_list_app_setting_values(uint8_t bd_addr[6],
                                   uint8_t attr_id)
{
    return false;
}

bool avrcp_get_current_app_setting_value(uint8_t  bd_addr[6],
                                         uint8_t  attr_num,
                                         uint8_t *p_attr)
{
    return false;
}

bool avrcp_set_app_setting_value(uint8_t              bd_addr[6],
                                 uint8_t              attr_num,
                                 T_AVRCP_APP_SETTING *p_app_setting)
{
    return false;
}

bool avrcp_req_continuing_rsp(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_abort_continuing_rsp(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_set_absolute_volume(uint8_t bd_addr[6],
                               uint8_t absolute_volume)
{
    return false;
}

bool avrcp_set_addressed_player(uint8_t  bd_addr[6],
                                uint16_t player_id)
{
    return false;
}

bool avrcp_set_addressed_player_rsp(uint8_t bd_addr[6],
                                    uint8_t status)
{
    return false;
}

bool avrcp_addressed_player_change_register_rsp(uint8_t  bd_addr[6],
                                                uint16_t player_id,
                                                uint16_t uid_counter)
{
    return false;
}

bool avrcp_notify_addressed_player_change(uint8_t  bd_addr[6],
                                          uint16_t player_id,
                                          uint16_t uid_counter)
{
    return false;
}

bool avrcp_play_item(uint8_t          bd_addr[6],
                     T_AVRCP_SCOPE_ID scope_id,
                     uint64_t         uid,
                     uint16_t         uid_counter)
{
    return false;
}

bool avrcp_navigate_group(uint8_t bd_addr[6],
                          bool    next,
                          bool    pressed)
{
    return false;
}

bool avrcp_send_vendor_cmd(uint8_t   bd_addr[6],
                           uint8_t   subunit_type,
                           uint8_t   subunit_id,
                           uint8_t   ctype,
                           uint32_t  company_id,
                           uint8_t  *p_pdu,
                           uint16_t  pdu_length)
{
    return false;
}

bool avrcp_send_vendor_rsp(uint8_t   bd_addr[6],
                           uint8_t   subunit_type,
                           uint8_t   subunit_id,
                           uint8_t   response,
                           uint32_t  company_id,
                           uint8_t  *p_pdu,
                           uint16_t  pdu_length)
{
    return false;
}

bool avrcp_browsing_connect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_browsing_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_browsing_connect_cfm(uint8_t bd_addr[6],
                                bool    accept)
{
    return false;
}

bool avrcp_browsing_get_folder_items(uint8_t                       bd_addr[6],
                                     T_AVRCP_REQ_GET_FOLDER_ITEMS *p_cmd_para)
{
    return false;
}

bool avrcp_browsing_get_folder_items_rsp(uint8_t   bd_addr[6],
                                         uint8_t   status,
                                         uint16_t  uid_counter,
                                         uint16_t  num_of_items,
                                         void     *p_items)
{
    return false;
}

bool avrcp_browsing_get_total_num_of_items_rsp(uint8_t  bd_addr[6],
                                               uint8_t  status,
                                               uint16_t uid_counter,
                                               uint32_t num_of_items)
{
    return false;
}

bool avrcp_browsing_get_item_attrs(uint8_t                     bd_addr[6],
                                   T_AVRCP_REQ_GET_ITEM_ATTRS *p_cmd_para)
{
    return false;
}

bool avrcp_browsing_search(uint8_t             bd_addr[6],
                           T_AVRCP_REQ_SEARCH *p_cmd_para)
{
    return false;
}

bool avrcp_browsing_set_browsed_player(uint8_t  bd_addr[6],
                                       uint16_t player_id)
{
    return false;
}

bool avrcp_browsing_change_path(uint8_t                  bd_addr[6],
                                T_AVRCP_REQ_CHANGE_PATH *p_cmd_para)
{
    return false;
}

bool avrcp_cover_art_conn_over_l2c(uint8_t  bd_addr[6],
                                   uint16_t l2c_psm)
{
    return false;
}

bool avrcp_cover_art_disconnect(uint8_t bd_addr[6])
{
    return false;
}

bool avrcp_cover_art_get_image(uint8_t  bd_addr[6],
                               uint8_t *img_handle)
{
    return false;
}

bool avrcp_cover_art_get_linked_thumbnail(uint8_t  bd_addr[6],
                                          uint8_t *img_handle)
{
    return false;
}
#endif
