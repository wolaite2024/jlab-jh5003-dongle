/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HFP_SUPPORT == 1)
#include <stdio.h>
#include <string.h>
#include "os_mem.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "sys_timer.h"
#include "rfc.h"
#include "hfp.h"

/* The mapping of indicator with name, cind response, type, and range */
const T_HFP_INDICATOR_NAME_TABLE INDICATOR_NAME_TABLE[HFP_AG_INDICATOR_COUNT] =
{
    { "service",    AG_INDICATOR_TYPE_SERVICE},
    { "call",       AG_INDICATOR_TYPE_CALL},
    { "callsetup",  AG_INDICATOR_TYPE_CALLSETUP},
    { "callheld",   AG_INDICATOR_TYPE_CALLHELD},
    { "signal",     AG_INDICATOR_TYPE_SIGNAL},
    { "roam",       AG_INDICATOR_TYPE_ROAM},
    { "battchg",    AG_INDICATOR_TYPE_BATTCHG},
};

typedef struct t_hfp_link
{
    void                   *timer_handle;
    uint8_t                *rx_ptr;
    uint16_t                cpbs;
    uint16_t                uuid; /* indicates hfp or hsp */
    uint16_t                supt_voice_codec;/* bit0:CVSD bit1:mSBC bit2~15 vendor */
    uint16_t                rx_len;
    uint8_t                 dlci;
    uint8_t                 credits;
    uint16_t                frame_size;
    uint8_t                 bd_addr[6];
    T_HFP_STATE             hfp_state;
    T_HFP_SRV_LEVEL_STEP    service_level_step;
    T_OS_QUEUE              cmd_queue; /* cmd from app api calls are buffered into this queue */
    T_INDICATOR_MAP         indicator_map; /*determined by the order of AG indicators */
} T_HFP_LINK;

typedef struct t_hfp
{
    P_HFP_HF_CBACK          cback;
    T_HFP_LINK             *link_list;
    uint16_t                brsf_cpbs;
    uint8_t                 hfp_rfc_index; /* rfomm index, which will be used when connect */
    uint8_t                 hsp_rfc_index; /* rfomm index, which will be used when connect */
    uint16_t                wait_rsp_tout;
    uint8_t                 link_num;
    uint16_t                supt_voice_codec; /*bit0:CVSD;bit1:mSBC,bit2~15 vendor*/
} T_HFP;

static T_HFP *p_hfp;

void hfp_tout_callback(T_SYS_TIMER_HANDLE handle);

T_HFP_LINK *hfp_alloc_link(uint8_t bd_addr[6])
{
    uint8_t     i;
    T_HFP_LINK *p_link = NULL;

    for (i = 0; i < p_hfp->link_num; i++)
    {
        if (p_hfp->link_list[i].hfp_state == HFP_STATE_DISCONNECTED)
        {
            p_link = &p_hfp->link_list[i];
            p_link->timer_handle = sys_timer_create("hfp_wait_rsp",
                                                    SYS_TIMER_TYPE_LOW_PRECISION,
                                                    (uint32_t)p_link,
                                                    p_hfp->wait_rsp_tout * 1000,
                                                    false,
                                                    hfp_tout_callback);
            if (p_link->timer_handle != NULL)
            {
                memcpy(p_link->bd_addr, bd_addr, 6);
                p_link->hfp_state = HFP_STATE_ALLOCATED;
                os_queue_init(&p_link->cmd_queue);
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

void hfp_free_link(T_HFP_LINK *p_link)
{
    if (p_link != NULL)
    {
        T_HFP_CMD_ITEM *cmd_item;

        cmd_item = os_queue_out(&p_link->cmd_queue);
        while (cmd_item)
        {
            os_mem_free(cmd_item);
            cmd_item = os_queue_out(&p_link->cmd_queue);
        }

        if (p_link->rx_ptr != NULL)
        {
            os_mem_free(p_link->rx_ptr);
        }

        if (p_link->timer_handle != NULL)
        {
            sys_timer_delete(p_link->timer_handle);
        }

        memset(p_link, 0, sizeof(T_HFP_LINK));
    }
}

T_HFP_LINK *hfp_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t     i;
    T_HFP_LINK *p_link = NULL;

    for (i = 0; i < p_hfp->link_num; i++)
    {
        if (!memcmp(p_hfp->link_list[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_hfp->link_list[i];
            break;
        }
    }
    return p_link;
}

uint8_t hfp_get_indicator_type_by_name(char *name)
{
    for (int i = 0; i < sizeof(INDICATOR_NAME_TABLE) / sizeof(INDICATOR_NAME_TABLE[0]); i++)
    {
        if (!strcmp(INDICATOR_NAME_TABLE[i].indicator_name, name))
        {
            return INDICATOR_NAME_TABLE[i].indicator_type;
        }
    }
    return AG_INDICATOR_TYPE_UNKOWN;
}

void hfp_update_ag_indicator_status(T_HFP_LINK *p_link,
                                    uint8_t     index,
                                    uint8_t     status)
{
    index--; /* begin with 0 */
    if (index < p_link->indicator_map.size)
    {
        T_HFP_MSG_AG_INDICATOR_EVENT message;
        memcpy(message.bd_addr, p_link->bd_addr, 6);
        message.event_type = (T_HFP_AG_INDICATOR_TYPE)(p_link->indicator_map.indicator[index].type);
        message.state = status;
        p_hfp->cback(p_link->bd_addr, HFP_MSG_AG_INDICATOR_EVENT, &message);
    }
}

bool hfp_send_cmd(T_HFP_LINK *p_link,
                  const char *at_cmd,
                  uint8_t     cmd_len)
{
    if (rfc_data_req(p_link->bd_addr, p_link->dlci, (uint8_t *)at_cmd,
                     cmd_len, false) == true)
    {
        p_link->credits = 0;
        sys_timer_start(p_link->timer_handle);
        return true;
    }
    else
    {
        PROFILE_PRINT_ERROR1("hfp_send_general_at_cmd: fail, len=%d", cmd_len);
        return false;
    }
}

bool hfp_flush_cmd(T_HFP_LINK *p_link)
{
    if ((p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTED) && (p_link->credits > 0))
    {
        T_HFP_CMD_ITEM *cmd_item;

        cmd_item = os_queue_peek(&p_link->cmd_queue, 0);
        if (cmd_item != NULL)
        {
            return hfp_send_cmd(p_link, cmd_item->at_cmd, cmd_item->cmd_len);
        }
    }
    return false;
}

bool hfp_cmd_process(uint8_t     bd_addr[6],
                     const char *at_cmd,
                     uint8_t     cmd_id)
{
    T_HFP_LINK      *p_link;
    int              cmd_len;
    T_HFP_CMD_ITEM  *cmd_item;
    int32_t          ret = 0;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    cmd_len = strlen(at_cmd);
    if ((cmd_len < 1) || (cmd_len > p_link->frame_size))
    {
        ret = 2;
        goto fail_invalid_cmd_length;
    }

    if (at_cmd[cmd_len - 1] != '\r')
    {
        ret = 3;
        goto fail_invalid_cmd_delimiter;
    }

    cmd_item = os_mem_zalloc2(sizeof(T_HFP_CMD_ITEM) + cmd_len);
    if (cmd_item == NULL)
    {
        ret = 4;
        goto fail_alloc_cmd_item;
    }

    cmd_item->cmd_id = cmd_id;
    cmd_item->cmd_len = cmd_len;
    memcpy((char *)cmd_item->at_cmd, at_cmd, cmd_len);

    os_queue_in(&p_link->cmd_queue, cmd_item);

    hfp_flush_cmd(p_link);
    return true;

fail_alloc_cmd_item:
fail_invalid_cmd_delimiter:
fail_invalid_cmd_length:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("hfp_cmd_process: bd_addr %s, failed %d",
                         TRACE_BDADDR(bd_addr), -ret);
    return false;
}

void hfp_bac_cmd_assemble(char *buf)
{
    uint8_t index = 8;
    uint8_t i;

    snprintf(buf, 9, "AT+BAC=1");

    for (i = 1; i < 16; i++)
    {
        if ((p_hfp->supt_voice_codec >> i) & 0x01)
        {
            if (i < 9)
            {
                snprintf(buf + index, 3, ",%u", (uint8_t)(i + 1));
                index += 2;
            }
            else
            {
                snprintf(buf + index, 4, ",%u", (uint8_t)(i + 1));
                index += 3;
            }
        }
    }

    snprintf(buf + index, 2, "\r");
}

void hfp_srv_level_established(T_HFP_LINK *p_link)
{
    p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTED;
    p_link->credits = 1;
    {
        T_HFP_MSG_CONN message;

        memcpy(message.bd_addr, p_link->bd_addr, 6);
        p_hfp->cback(p_link->bd_addr, HFP_MSG_CONN, &message);
    }
    hfp_flush_cmd(p_link);
}

bool hfp_srv_level_step(T_HFP_LINK *p_link,
                        bool        result_ok)
{
    char buf[47];

    while (1)
    {
        p_link->service_level_step++;
        switch (p_link->service_level_step)
        {
        case HFP_SRV_LEVEL_BRSF: /* mandatory */
            {
                snprintf(buf, 15, "AT+BRSF=%u\r", p_hfp->brsf_cpbs);
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_BAC: /* mandatory */
            if ((p_link->cpbs & AG_CAPABILITY_CODEC_NEGOTIATION) &&
                (p_hfp->brsf_cpbs & HF_CAPABILITY_CODEC_NEGOTIATION))
            {
                hfp_bac_cmd_assemble(buf);
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }
            else
            {
                continue;
            }

        case HFP_SRV_LEVEL_CIND_TEST: /* mandatory */
            {
                snprintf(buf, 11, "AT+CIND=?\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_CIND_INQUIRY: /* mandatory */
            /* The AT+CIND? read command is used to get current status of the AG indicators. */
            {
                snprintf(buf, 10, "AT+CIND?\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_ACTIVATE_INDICATOR: /* mandatory */
            /*  AT+CMER=3,0,0,1 activates indicator events reporting.
                AT+CMER=3,0,0,0 deactivates indicator events reporting.*/
            {
                snprintf(buf, 17, "AT+CMER=3,0,0,1\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }

        case HFP_SRV_LEVEL_CHLD_TEST: /* mandatory, after this ServiceLevelConnection is Established */
            if ((p_link->cpbs & AG_CAPABILITY_3WAY) &&
                (p_hfp->brsf_cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY))
            {
                /* get call hold capabilities */
                snprintf(buf, 11, "AT+CHLD=?\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }
            else
            {
                continue;
            }

        case HFP_SRV_LEVEL_BIND:
            if ((p_link->cpbs & AG_CAPABILITY_HF_INDICATORS) &&
                (p_hfp->brsf_cpbs & HF_CAPABILITY_HF_INDICATORS))
            {
                snprintf(buf, 11, "AT+BIND=2\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }
            else
            {
                p_link->service_level_step += 2;
                continue;
            }

        case HFP_SRV_LEVEL_BIND_TEST:
            if (result_ok == true)
            {
                snprintf(buf, 11, "AT+BIND=?\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }
            else
            {
                p_link->service_level_step++;
                continue;
            }

        case HFP_SRV_LEVEL_BIND_INQUIRY:
            {
                snprintf(buf, 10, "AT+BIND?\r");
                return hfp_send_cmd(p_link, buf, strlen(buf));
            }

        default:
            return false;
        }
    }
}

void hfp_srv_level_continue(T_HFP_LINK *p_link,
                            bool        result_ok)
{
    if (p_link->uuid == UUID_HANDSFREE)
    {
        if (false == hfp_srv_level_step(p_link, result_ok))
        {
            hfp_srv_level_established(p_link);
        }
    }
    else
    {
        hfp_srv_level_established(p_link);
    }
}

void hfp_srv_level_start(T_HFP_LINK *p_link)
{
    p_link->service_level_step = HFP_SRV_LEVEL_NONE;
    hfp_srv_level_continue(p_link, true);
}

void hfp_handle_ok(T_HFP_LINK *p_link,
                   uint8_t     cmd_id)
{
    T_HFP_MSG_ACK_OK message;

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    message.last_cmd_id = cmd_id;
    p_hfp->cback(p_link->bd_addr, HFP_MSG_ACK_OK, &message);
}

void hfp_handle_error(T_HFP_LINK *p_link,
                      uint8_t     cmd_id)
{
    T_HFP_MSG_ACK_ERROR message;

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    message.last_cmd_id = cmd_id;
    p_hfp->cback(p_link->bd_addr, HFP_MSG_ACK_ERROR, &message);
}

int32_t hfp_handle_cme_error(T_HFP_LINK *p_link,
                             const char *str,
                             uint8_t     cmd_id)
{
    int res;
    int offset;
    T_HFP_MSG_CME_ERROR message;
    int32_t ret = 0;

    res = sscanf(str, "+CME ERROR: %hhu%n", &message.error_number, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    message.last_cmd_id = cmd_id;
    p_hfp->cback(p_link->bd_addr, HFP_MSG_CME_ERROR, &message);

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_brsf(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    int32_t ret = 0;

    res = sscanf(str, "+BRSF: %hu%n", &p_link->cpbs, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_cind(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    int32_t ret = 0;

    str += 6;
    if (*str == ' ')
    {
        str++;
    }

    if (*str == '(')
    {
        char name[20];
        uint8_t max;

        while ((res = sscanf(str, "(\"%19[^\"]\",(0%*[-,]%hhu))%n", name, &max, &offset)) == 2)
        {
            p_link->indicator_map.indicator[p_link->indicator_map.size].type =
                hfp_get_indicator_type_by_name(name);
            p_link->indicator_map.indicator[p_link->indicator_map.size].range = max;

            str += offset;
            p_link->indicator_map.size++;

            if (*str != ',')
            {
                break;
            }

            str++;
        }

        if (res < 2)
        {
            ret = 1;
            goto fail_cind_list_value;
        }

        if (*str != '\0')
        {
            ret = 2;
            goto fail_cind_list_end_part;
        }
    }
    else
    {
        uint8_t index = 1;
        uint8_t value = 0;

        while ((res = sscanf(str, "%hhu%n", &value, &offset)) == 1)
        {
            hfp_update_ag_indicator_status(p_link, index, value);

            str += offset;

            /* check if more values are present */
            if (*str != ',')
            {
                break;
            }

            index++;
            str++;
        }

        if (res < 1)
        {
            ret = 3;
            goto fail_cind_item_value;
        }

        if (*str != '\0')
        {
            ret = 4;
            goto fail_cind_item_end_part;
        }
    }

    return 0;

fail_cind_item_end_part:
fail_cind_item_value:
fail_cind_list_end_part:
fail_cind_list_value:
    return ret;
}

int32_t hfp_handle_clcc(T_HFP_LINK *p_link,
                        const char *str)
{
    /*
    <idx>,<dir>,<status>,<mode>,<mprty>[,<number>,<type>]
    "+CLCC: 1,0,3,0,0,"10010",129"
    */
    T_HFP_MSG_CLCC msg;
    int            res;
    int            offset;
    int32_t        ret = 0;

    msg.type = 128;
    memset(msg.number, 0, sizeof(msg.number));
    memcpy(msg.bd_addr, p_link->bd_addr, 6);

    res = sscanf(str, "+CLCC: %hhu,%hhu,%hhu,%hhu,%hhu%n", &msg.call_idx, &msg.dir_incoming,
                 &msg.status, &msg.mode, &msg.mpty, &offset);
    if (res < 5)
    {
        ret = 1;
        goto fail_mandatory_part;
    }

    str += offset;
    offset = 0;

    if (*str == ',')
    {
        /* check optional part */
        res = sscanf(str, ",\"%20[^\"]\",%hhu%n", msg.number, &msg.type, &offset);
        if (res == 0)
        {
            sscanf(str, ",\"\",%hhu%n", &msg.type, &offset);
        }
    }

    p_hfp->cback(p_link->bd_addr, HFP_MSG_CLCC, &msg);
    return 0;

fail_mandatory_part:
    return ret;
}

void hfp_handle_ring(T_HFP_LINK *p_link)
{
    T_HFP_MSG_RING message;

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    p_hfp->cback(p_link->bd_addr, HFP_MSG_RING, &message);
}

int32_t hfp_handle_bvra(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    T_HFP_MSG_VOICE_RECOGNITION_STATUS message;
    int32_t ret = 0;

    res = sscanf(str, "+BVRA: %hhu%n", &message.status, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    p_hfp->cback(p_link->bd_addr, HFP_MSG_VOICE_RECOGNITION_STATUS, &message);
    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_clip(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    T_HFP_MSG_CLIP message;
    int32_t ret = 0;

    memset(message.number, 0, sizeof(message.number));
    memcpy(message.bd_addr, p_link->bd_addr, 6);

    res = sscanf(str, "+CLIP: \"%20[^\"]\",%hhu%n", message.number, &message.type, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if ((*(str + offset) != ',') &&
        (*(str + offset) != '\0'))
    {
        ret = 2;
        goto fail_end_part;
    }

    p_hfp->cback(p_link->bd_addr, HFP_MSG_CLIP, &message);
    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_ciev(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    uint8_t index;
    uint8_t value;
    int32_t ret = 0;

    res = sscanf(str, "+CIEV: %hhu,%hhu%n", &index, &value, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    hfp_update_ag_indicator_status(p_link, index, value);
    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_ccwa(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    T_HFP_MSG_CCWA message;
    int32_t ret = 0;

    memset(message.number, 0, sizeof(message.number));
    memcpy(message.bd_addr, p_link->bd_addr, 6);

    res = sscanf(str, "+CCWA: \"%20[^\"]\",%hhu%n", message.number, &message.type, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    p_hfp->cback(p_link->bd_addr, HFP_MSG_CCWA, &message);
    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_vgm(T_HFP_LINK *p_link,
                       const char *str)
{
    int res;
    int offset;
    T_HFP_MSG_MICROPHONE_VOLUME_SET message;
    int32_t ret = 0;

    res = sscanf(str, "+VGM%*[:=]%hhu%n", &message.volume, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if ((p_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
    {
        ret = 3;
        goto fail_support_event;
    }

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    p_hfp->cback(p_link->bd_addr, HFP_MSG_SET_MICROPHONE_GAIN, &message);
    return 0;

fail_support_event:
fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_vgs(T_HFP_LINK *p_link,
                       const char *str)
{
    int res;
    int offset;
    T_HFP_MSG_SPEAKER_VOLUME_SET message;
    int32_t ret = 0;

    res = sscanf(str, "+VGS%*[:=]%hhu%n", &message.volume, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if ((p_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
    {
        ret = 3;
        goto fail_support_event;
    }

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    p_hfp->cback(p_link->bd_addr, HFP_MSG_SET_SPEAKER_GAIN, &message);
    return 0;

fail_support_event:
fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_bcs(T_HFP_LINK *p_link,
                       const char *str)
{
    char buf[47];
    int res;
    int offset;
    uint8_t codec_type;
    int32_t ret = 0;

    res = sscanf(str, "+BCS: %hhu%n", &codec_type, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if (p_hfp->supt_voice_codec >> (codec_type - 1) & 0x01)
    {
        T_HFP_MSG_SET_CODEC_TYPE message;

        memcpy(message.bd_addr, p_link->bd_addr, 6);
        message.codec_type = (T_HFP_CODEC_TYPE)codec_type;
        p_hfp->cback(p_link->bd_addr, HFP_MSG_SET_CODEC_TYPE, &message);

        snprintf(buf, 12, "AT+BCS=%u\r", codec_type);
        hfp_cmd_process(p_link->bd_addr, buf, HFP_CMD_ID_BCS);
    }
    else /* If the received ID is not available, the HF shall respond with AT+BAC with its available codecs. */
    {
        hfp_bac_cmd_assemble(buf);
        hfp_cmd_process(p_link->bd_addr, buf, HFP_CMD_ID_BAC);
    }

    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_bsir(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    uint8_t value;
    T_HFP_MSG_AG_INBAND_RINGTONE_SET message;
    int32_t ret = 0;

    res = sscanf(str, "+BSIR: %hhu%n", &value, &offset);
    if (res < 1)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    memcpy(message.bd_addr, p_link->bd_addr, 6);
    message.ag_support = value ? true : false;
    p_hfp->cback(p_link->bd_addr, HFP_MSG_AG_INBAND_RINGTONE_SETTING, &message);
    return 0;

fail_end_part:
fail_no_value:
    return ret;
}

int32_t hfp_handle_bind(T_HFP_LINK *p_link,
                        const char *str)
{
    int res;
    int offset;
    T_HFP_MSG_BIND_STATUS message;
    uint8_t indicator;
    uint8_t state;
    int32_t ret = 0;

    str += 6;
    if (*str == ' ')
    {
        str++;
    }

    /* Response to AT+BIND=? format : +BIND: (<a>,<b>,<c>,...,<n>)
       or to AT+BIND? format : +BIND: <a>,<state> */
    if ((*str != '('))
    {
        /* Unsolicited or Response to AT+BIND? format : +BIND: <a>,<state> */
        res = sscanf(str, " %hhu,%hhu%n", &indicator, &state, &offset);
        if (res < 2)
        {
            ret = 1;
            goto fail_no_bind_item_value;
        }

        if (*(str + offset) != '\0')
        {
            ret = 2;
            goto fail_bind_item_end_part;
        }

        if (indicator == HF_BIND_BATTERY_LEVEL)
        {
            memcpy(message.bd_addr, p_link->bd_addr, 6);
            message.batt_ind_enable = state == 1 ? true : false;
            p_hfp->cback(p_link->bd_addr, HFP_MSG_AG_BIND, &message);
        }
    }

    return 0;

fail_bind_item_end_part:
fail_no_bind_item_value:
    return ret;
}

int32_t hfp_handle_cops(T_HFP_LINK *p_link,
                        const char *str)
{
    T_HFP_MSG_COPS msg;
    int            res;
    int            offset;
    int32_t        ret = 0;

    msg.mode = 0;
    msg.format = 0;
    memset(msg.operator, 0, sizeof(msg.operator));
    memcpy(msg.bd_addr, p_link->bd_addr, 6);

    res = sscanf(str, "+COPS: %hhu,0,\"%16[^\"]\"%n", &msg.mode, msg.operator, &offset);
    if (res <= 0)
    {
        ret = 1;
        goto fail_no_mode_operator_name;
    }

    if (res == 1)
    {
        offset = 0;
        res = sscanf(str, "+COPS: %hhu,0,\"\"%n", &msg.mode, &offset);
        if (res <= 0)
        {
            ret = 2;
            goto fail_no_mode_null_operator_name;
        }

        /* filter "+COPS: 1,0,\"" when res is 1. */
        if (offset == 0)
        {
            ret = 3;
            goto fail_mode_no_null_operator_name;
        }
    }

    str += offset;
    if (*str != '\0')
    {
        ret = 4;
        goto fail_end_part;
    }

    p_hfp->cback(p_link->bd_addr, HFP_MSG_COPS, &msg);
    return 0;

fail_end_part:
fail_mode_no_null_operator_name:
fail_no_mode_null_operator_name:
fail_no_mode_operator_name:
    return ret;
}

int32_t hfp_handle_cnum(T_HFP_LINK *p_link,
                        const char *str)
{
    T_HFP_MSG_CNUM msg;
    int            res;
    int            offset;
    int32_t        ret = 0;

    msg.type = 128;
    msg.service = 4;
    memset(msg.number, 0, sizeof(msg.number));
    memcpy(msg.bd_addr, p_link->bd_addr, 6);

    res = sscanf(str, "+CNUM: ,\"%20[^\"]\",%hhu%n", msg.number, &msg.type, &offset);
    if (res < 0)
    {
        ret = 1;
        goto fail_no_number;
    }

    if (res == 0)
    {
        res = sscanf(str, "+CNUM: ,\"\",%hhu%n", &msg.type, &offset);
        if (res <= 0)
        {
            ret = 2;
            goto fail_null_number_no_type;
        }
    }
    else if (res == 1)
    {
        ret = 3;
        goto fail_number_no_type;
    }

    str += offset;
    offset = 0;

    if (*str == ',')
    {
        /* check optional part */
        res = sscanf(str, ",,%hhu%n", &msg.service, &offset);
        if (res < 1)
        {
            ret = 4;
            goto fail_no_service;
        }
    }

    str += offset;
    if (*str != '\0')
    {
        ret = 5;
        goto fail_end_part;
    }

    p_hfp->cback(p_link->bd_addr, HFP_MSG_CNUM, &msg);
    return 0;

fail_end_part:
fail_no_service:
fail_number_no_type:
fail_null_number_no_type:
fail_no_number:
    return ret;
}

void hfp_handle_at_rsp(T_HFP_LINK *p_link,
                       const char *at_response)
{
    char    *at_rsp;
    int32_t  ret = 0;

    if (!strncmp("OK", at_response, 2) ||
        !strncmp("ERROR", at_response, 5) ||
        !strncmp("+CME ERROR", at_response, 10))
    {
        /* Hayes processing is finished */
        sys_timer_stop(p_link->timer_handle);
        if (p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTED)
        {
            T_HFP_CMD_ITEM *cmd_item;

            p_link->credits = 1;

            cmd_item = os_queue_out(&p_link->cmd_queue);
            if (cmd_item != NULL)
            {
                if (!strncmp("OK", at_response, 2))
                {
                    hfp_handle_ok(p_link, cmd_item->cmd_id);
                }
                else if (!strncmp("ERROR", at_response, 5))
                {
                    hfp_handle_error(p_link, cmd_item->cmd_id);
                }
                else /* "+CME ERROR:", only when +CME ERROR is enabled by AT+CMEE=1 */
                {
                    at_rsp = "+CME ERROR";
                    ret = hfp_handle_cme_error(p_link, at_response, cmd_item->cmd_id);
                }

                os_mem_free(cmd_item);
                hfp_flush_cmd(p_link);
            }
        }
        else if (p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTING)
        {
            if (!strncmp("OK", at_response, 2))
            {
                hfp_srv_level_continue(p_link, true);
            }
            else
            {
                hfp_srv_level_continue(p_link, false);
            }
        }
    }
    else
    {
        /* there is an inconsistency in the spec between gsm 07.07 and hs / hf
        hs defines '=' as the separator, hf defines the ':' as the separator
        therefore we completely ignore the separator on the incoming side.
        */
        /*Iphone send +BRSF:<AG supported features bitmap> with no space*/
        if (!strncmp("+BRSF", at_response, 5))
        {
            at_rsp = "+BRSF";
            ret = hfp_handle_brsf(p_link, at_response);
        }
        else if (!strncmp("+CIND:", at_response, 6))
        {
            at_rsp = "+CIND:";
            ret = hfp_handle_cind(p_link, at_response);
        }
        else if (!strncmp("+CHLD", at_response, 5))
        {
            /* Parse a capability list.
             * GSM Spec ETS 300 916:
             * It is recommended (although optional) that test command returns a list of
             * operations which are supported.
             * The call number required by some operations shall be denoted by "x" (e.g. +CHLD: (0,1,1x,2,2x,3)).
             */
        }
        else if (!strncmp("+CLCC", at_response, 5))
        {
            at_rsp = "+CLCC";
            ret = hfp_handle_clcc(p_link, at_response);
        }
        else if (!strncmp("RING", at_response, 4))
        {
            hfp_handle_ring(p_link);
        }
        else if (!strncmp("+BVRA", at_response, 5))
        {
            at_rsp = "+BVRA";
            ret = hfp_handle_bvra(p_link, at_response);
        }
        else if (!strncmp("+CLIP", at_response, 5))
        {
            at_rsp = "+CLIP";
            ret = hfp_handle_clip(p_link, at_response);
        }
        else if (!strncmp("+CIEV", at_response, 5))
        {
            at_rsp = "+CIEV";
            ret = hfp_handle_ciev(p_link, at_response);
        }
        else if (!strncmp("+CCWA", at_response, 5))
        {
            at_rsp = "+CCWA";
            ret = hfp_handle_ccwa(p_link, at_response);
        }
        else if (!strncmp("+VGM", at_response, 4))
        {
            at_rsp = "+VGM";
            ret = hfp_handle_vgm(p_link, at_response);
        }
        else if (!strncmp("+VGS", at_response, 4))
        {
            at_rsp = "+VGS";
            ret = hfp_handle_vgs(p_link, at_response);
        }
        else if (!strncmp("+BCS", at_response, 4))
        {
            at_rsp = "+BCS";
            ret = hfp_handle_bcs(p_link, at_response);
        }
        else if (!strncmp("+BSIR", at_response, 5))
        {
            at_rsp = "+BSIR";
            ret = hfp_handle_bsir(p_link, at_response);
        }
        else if (!strncmp("+BIND:", at_response, 6))
        {
            at_rsp = "+BIND";
            ret = hfp_handle_bind(p_link, at_response);
        }
        else if (!strncmp("+COPS", at_response, 5))
        {
            at_rsp = "+COPS";
            ret = hfp_handle_cops(p_link, at_response);
        }
        else if (!strncmp("+CNUM", at_response, 5))
        {
            at_rsp = "+CNUM";
            ret = hfp_handle_cnum(p_link, at_response);
        }
        else /* unkown AT command */
        {
            T_HFP_MSG_UNKNOWN_CMD message;

            memcpy(message.bd_addr, p_link->bd_addr, 6);
            message.at_cmd = (char *)at_response;
            p_hfp->cback(p_link->bd_addr, HFP_MSG_UNKNOWN_CMD, &message);
        }
    }

    if (ret != 0)
    {
        PROFILE_PRINT_ERROR2("hfp_handle_at_rsp: %s, failed %d", TRACE_STRING(at_rsp), -ret);
    }
}

bool hfp_dial_with_number(uint8_t     bd_addr[6],
                          const char *number)
{
    char buf[25];

    snprintf(buf, 25, "ATD%s;\r", number);
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_DIAL_WITH_NUMBER);
}

bool hfp_dial_last_number(uint8_t bd_addr[6])
{
    char buf[9];

    snprintf(buf, 9, "AT+BLDN\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_DIAL_LAST_NUMBER);
}

bool hfp_dtmf_send(uint8_t bd_addr[6],
                   char    c)
{
    char buf[10];

    snprintf(buf, 10, "AT+VTS=%c\r", c);
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_SEND_DTMF);
}

bool hfp_accept_phone_call(uint8_t bd_addr[6])
{
    char buf[5];

    snprintf(buf, 5, "ATA\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_ACCEPT_CALL);
}

bool hfp_reject_phone_call(uint8_t bd_addr[6])
{
    char buf[9];

    snprintf(buf, 9, "AT+CHUP\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_REJECT_HANGUP_CALL);
}

bool hfp_send_nrec_disable(uint8_t bd_addr[6])
{
    char     buf[11];

    if (hfp_local_capabilities_get() & HF_CAPABILITY_EC_NR)
    {
        snprintf(buf, 11, "AT+NREC=0\r");
        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_NREC);
    }
    return false;
}

bool hfp_send_cmee_enable(uint8_t bd_addr[6])
{
    T_HFP_LINK *p_link;
    char        buf[11];

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->cpbs & AG_CAPABILITY_EXTENED_ERROR_RESULT)
        {
            snprintf(buf, 11, "AT+CMEE=1\r");
            return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_CMEE);
        }
    }
    return false;
}

bool hfp_send_ccwa(uint8_t bd_addr[6])
{
    char     buf[11];

    if (hfp_local_capabilities_get() & HF_CAPABILITY_CALL_WAITING_OR_3WAY)
    {
        snprintf(buf, 11, "AT+CCWA=1\r");
        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_CCWA);
    }

    return false;
}

bool hfp_send_clip(uint8_t bd_addr[6])
{
    char     buf[11];

    if (hfp_local_capabilities_get() & HF_CAPABILITY_CLI)
    {
        snprintf(buf, 11, "AT+CLIP=1\r");
        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_CLIP);
    }
    return false;
}

bool hfp_send_clcc(uint8_t bd_addr[6])
{
    char     buf[9];

    if (hfp_local_capabilities_get() & HF_CAPABILITY_ENHANCED_CALL_STATUS)
    {
        snprintf(buf, 9, "AT+CLCC\r");
        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_SEND_CLCC);
    }
    return false;
}

bool hfp_inform_ag_microphone_gain(uint8_t bd_addr[6],
                                   uint8_t level)
{
    if (p_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL)
    {
        char buf[11];

        snprintf(buf, 11, "AT+VGM=%u\r", level);
        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_INFORM_MICROPHONE_GAIN);
    }

    return false;
}

bool hfp_inform_ag_speaker_gain(uint8_t bd_addr[6],
                                uint8_t level)
{
    if (p_hfp->brsf_cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL)
    {
        char buf[11];

        snprintf(buf, 11, "AT+VGS=%u\r", level);
        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_INFORM_SPEAKER_GAIN);
    }

    return false;
}

bool hfp_set_voice_recognition(uint8_t bd_addr[6],
                               bool    enable)
{
    char buf[11];

    snprintf(buf, 11, "AT+BVRA=%u\r", enable);
    return hfp_cmd_process(bd_addr, buf, enable ? HFP_CMD_ID_SET_VOICE_RECOGNITION_ACTIVE :
                           HFP_CMD_ID_SET_VOICE_RECOGNITION_INACTIVE);
}

bool hfp_call_hold_action(uint8_t                bd_addr[6],
                          T_HFP_CALL_HOLD_ACTION control)
{
    uint8_t app_cmd_id = 0;
    char    buf[11];

    snprintf(buf, 11, "AT+CHLD=%u\r", (uint8_t)control);
    if (control == RELEASE_HELD_OR_WAITING_CALL)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_0;
    }
    else if (control == RELEASE_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_1;
    }
    else if (control == HOLD_ACTIVE_CALL_ACCEPT_HELD_OR_WAITING_CALL)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_2;
    }
    else if (control == JOIN_TWO_CALLS)
    {
        app_cmd_id = HFP_CMD_ID_3WAY_CALL_CONTROL_3;
    }

    return hfp_cmd_process(bd_addr, buf, app_cmd_id);
}

bool hfp_3way_call_control_with_index(uint8_t bd_addr[6],
                                      uint8_t control,
                                      uint8_t index)
{
    char buf[14];

    snprintf(buf, 14, "AT+CHLD=%u%u\r", control, index);
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_3WAY_CALL_CONTROL_AB);
}

bool hfp_hsp_button_press(uint8_t bd_addr[6])
{
    char buf[13];

    snprintf(buf, 13, "AT+CKPD=200\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_HSP_BUTTON_PRESS);
}

bool hfp_ask_ag_establish_voice_chann(uint8_t bd_addr[6])
{
    char buf[8];

    snprintf(buf, 8, "AT+BCC\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_ESTABLISH_VOICE);
}

bool hfp_network_operator_format_set(uint8_t bd_addr[6])
{
    char buf[13];

    snprintf(buf, 13, "AT+COPS=3,0\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_SET_NETWORK_OPERATOR_FORMAT);
}

bool hfp_network_operator_name_query(uint8_t bd_addr[6])
{
    char buf[10];

    snprintf(buf, 10, "AT+COPS?\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_QUERY_NETWORK_NAME);
}

bool hfp_subscriber_num_query(uint8_t bd_addr[6])
{
    char buf[9];

    snprintf(buf, 9, "AT+CNUM\r");
    return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_QUERY_SUBSCRIBER_NUM);
}

bool hfp_indicator_activate(uint8_t  bd_addr[6],
                            uint32_t indicator_types)
{
    char        buf[25];
    uint8_t     index;
    uint8_t     control[7] = {0};
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        for (index = 0; index < p_link->indicator_map.size; index++)
        {
            if (indicator_types & (0x01 << p_link->indicator_map.indicator[index].type))
            {
                control[index] = 1;
            }
        }

        snprintf(buf, 25, "AT+BIA=%u,%u,%u,%u,%u,%u,%u\r",
                 control[0],
                 control[1],
                 control[2],
                 control[3],
                 control[4],
                 control[5],
                 control[6]);

        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_INDICATOR_ACTIVATE);
    }

    return false;
}

bool hfp_indicator_deactivate(uint8_t  bd_addr[6],
                              uint32_t indicator_types)
{
    char        buf[25];
    uint8_t     index;
    uint8_t     control[7] = {1};
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        for (index = 0; index < p_link->indicator_map.size; index++)
        {
            if (indicator_types & (0x01 << p_link->indicator_map.indicator[index].type))
            {
                control[index] = 0;
            }
        }

        snprintf(buf, 25, "AT+BIA=%u,%u,%u,%u,%u,%u,%u\r",
                 control[0],
                 control[1],
                 control[2],
                 control[3],
                 control[4],
                 control[5],
                 control[6]);

        return hfp_cmd_process(bd_addr, buf, HFP_CMD_ID_INDICATOR_DEACTIVATE);
    }

    return false;
}

bool hfp_batt_level_report(uint8_t     bd_addr[6],
                           const char *at_cmd,
                           uint8_t     type)
{
    uint8_t cmd_id = 0;

    switch (type)
    {
    case BAT_REPORTING_TYPE_NONE:
        {
            cmd_id = HFP_CMD_ID_BATT_XAPL;
        }
        break;

    case BAT_REPORTING_TYPE_BIEV:
        {
            cmd_id = HFP_CMD_ID_BATT_BIEV;
        }
        break;

    case BAT_REPORTING_TYPE_APPLE:
        {
            cmd_id = HFP_CMD_ID_BATT_IPHONEACCEV;
        }
        break;

    case BAT_REPORTING_TYPE_ANDROID:
        {
            cmd_id = HFP_CMD_ID_BATT_XEVENT;
        }
        break;

    default:
        return false;
    }

    return hfp_cmd_process(bd_addr, at_cmd, cmd_id);
}

bool hfp_send_vnd_cmd(uint8_t     bd_addr[6],
                      const char *at_cmd)
{
    return hfp_cmd_process(bd_addr, at_cmd, HFP_CMD_ID_VND_AT_CMD);
}

void hfp_handle_rfc_conn_cmpl(uint8_t  bd_addr[6],
                              uint8_t  index,
                              uint8_t  dlci,
                              uint8_t  credits,
                              uint16_t frame_size)
{
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        rfc_disconn_req(bd_addr, dlci);
        return;
    }

    if (index == p_hfp->hfp_rfc_index)
    {
        p_link->uuid = UUID_HANDSFREE;
    }
    else if (index == p_hfp->hsp_rfc_index)
    {
        p_link->uuid = UUID_HEADSET;
    }
    else
    {
        PROFILE_PRINT_ERROR1("hfp_handle_rfc_conn_cmpl: wrong index[%d] received", index);
        return;
    }

    PROFILE_PRINT_INFO4("hfp_handle_rfc_conn_cmpl: bd_addr[%s], dlci=%d, tx_credits =%d, uuid=%x[hfp:111e/hsp:1108]",
                        TRACE_BDADDR(bd_addr), dlci, credits, p_link->uuid);
    p_link->frame_size = frame_size;
    {
        T_HFP_MSG_RFC_CONN message;
        memcpy(message.bd_addr, p_link->bd_addr, 6);
        message.hsp = p_link->uuid == UUID_HEADSET ? true : false;
        p_hfp->cback(p_link->bd_addr, HFP_MSG_RFC_CONN, &message);
    }

    if (credits == 0) /* when connected credits may = 0, wait till credits > 0 */
    {
        p_link->hfp_state = HFP_STATE_RFC_CONN_NO_CREDITS;
    }
    else
    {
        p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
        hfp_srv_level_start(p_link);
    }
}

void hfp_handle_rfc_data_ind(T_RFC_DATA_IND *p_data_msg)
{
    uint8_t    *p_data;
    uint8_t    *temp_ptr = NULL;
    uint16_t    length;
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(p_data_msg->bd_addr);
    if (p_link == NULL)
    {
        return;
    }

    if (p_link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && p_data_msg->remain_credits > 0)
    {
        p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
        hfp_srv_level_start(p_link);
    }

    if (p_data_msg->length == 0)
    {
        return;
    }

    if (p_link->rx_ptr != NULL)
    {
        temp_ptr = p_link->rx_ptr;
        p_link->rx_ptr = os_mem_alloc2((p_link->rx_len + p_data_msg->length));
        if (p_link->rx_ptr == NULL)
        {
            return;
        }
        memcpy(p_link->rx_ptr, temp_ptr, p_link->rx_len);
        os_mem_free(temp_ptr);
        memcpy(&(p_link->rx_ptr[p_link->rx_len]), p_data_msg->buf, p_data_msg->length);
        p_data = p_link->rx_ptr;
        length = p_link->rx_len + p_data_msg->length;
    }
    else
    {
        p_data = p_data_msg->buf;
        length = p_data_msg->length;
    }
    p_link->rx_len = 0;

    while (length)
    {
        if ((*p_data > ' ') || ((*p_data == ' ') && (p_link->rx_len > 0)))
        {
            if (p_link->rx_len == 0)
            {
                temp_ptr = p_data;
            }
            p_link->rx_len++;
        }
        else if ((*p_data == '\r') && (p_link->rx_len > 0))
        {
            *p_data = '\0';
            hfp_handle_at_rsp(p_link, (const char *)temp_ptr);
            p_link->rx_len = 0;
        }
        p_data++;
        length--;
    }

    p_data = p_link->rx_ptr;
    p_link->rx_ptr = NULL;
    if (p_link->rx_len)
    {
        p_link->rx_ptr = os_mem_alloc2(p_link->rx_len);
        if (p_link->rx_ptr == NULL)
        {
            return;
        }
        memcpy(p_link->rx_ptr, temp_ptr, p_link->rx_len);
    }
    if (p_data != NULL)
    {
        os_mem_free(p_data);
    }

    rfc_data_cfm(p_data_msg->bd_addr, p_data_msg->dlci, 1);

    return;
}

void hfp_handle_rfc_msg(T_RFC_MSG_TYPE  msg_type,
                        void           *p_msg)
{
    PROFILE_PRINT_TRACE1("HFP receive rfc msg_type=%x", msg_type);

    switch (msg_type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *p_conn_ind_msg = (T_RFC_CONN_IND *)p_msg;
            T_HFP_LINK *p_link = hfp_find_link_by_addr(p_conn_ind_msg->bd_addr);

            if (p_link == NULL)
            {
                p_link = hfp_alloc_link(p_conn_ind_msg->bd_addr);
                if (p_link == NULL)
                {
                    rfc_conn_cfm(p_conn_ind_msg->bd_addr, p_conn_ind_msg->dlci, RFC_REJECT,
                                 p_conn_ind_msg->frame_size, RFC_DEFAULT_CREDIT);
                }
                else
                {
                    p_link->dlci = p_conn_ind_msg->dlci;
                    p_link->frame_size = p_conn_ind_msg->frame_size;
                    p_hfp->cback(p_link->bd_addr, HFP_MSG_RFC_CONN_IND, p_conn_ind_msg->bd_addr);
                }
            }
            else
            {
                rfc_conn_cfm(p_conn_ind_msg->bd_addr, p_conn_ind_msg->dlci, RFC_REJECT,
                             p_conn_ind_msg->frame_size, RFC_DEFAULT_CREDIT);
            }
        }
        break;

    case RFC_CONN_CMPL:
        {
            T_RFC_CONN_CMPL *p_complete_msg = (T_RFC_CONN_CMPL *)p_msg;
            hfp_handle_rfc_conn_cmpl(p_complete_msg->bd_addr, p_complete_msg->profile_index,
                                     p_complete_msg->dlci, p_complete_msg->remain_credits,
                                     p_complete_msg->frame_size);
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *p_disc_msg = (T_RFC_DISCONN_CMPL *)p_msg;
            T_HFP_LINK *p_link = hfp_find_link_by_addr(p_disc_msg->bd_addr);

            if (p_link != NULL)
            {
                if (p_link->dlci == p_disc_msg->dlci)
                {
                    if (p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTED)
                    {
                        T_HFP_DISCONN_INFO info;
                        info.cause = p_disc_msg->cause;
                        p_hfp->cback(p_link->bd_addr, HFP_MSG_DISCONN, &info);
                    }
                    else
                    {
                        uint16_t cause = p_disc_msg->cause;
                        p_hfp->cback(p_link->bd_addr, HFP_MSG_CONN_FAIL, &cause);
                    }

                    hfp_free_link(p_link);
                }
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *p_credit_msg = (T_RFC_CREDIT_INFO *)p_msg;
            T_HFP_LINK *p_link = hfp_find_link_by_addr(p_credit_msg->bd_addr);
            if (p_link != NULL)
            {
                if (p_link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && p_credit_msg->remain_credits > 0)
                {
                    p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
                    hfp_srv_level_start(p_link);
                }
            }
        }
        break;

    case RFC_DATA_IND:
        hfp_handle_rfc_data_ind((T_RFC_DATA_IND *)p_msg);
        break;

    case RFC_DLCI_CHANGE:
        {
            T_RFC_DLCI_CHANGE_INFO *p_info = (T_RFC_DLCI_CHANGE_INFO *)p_msg;
            T_HFP_LINK *p_link;

            p_link = hfp_find_link_by_addr(p_info->bd_addr);
            if (p_link)
            {
                p_link->dlci = p_info->curr_dlci;
            }
        }
        break;

    default:
        break;
    }
}

bool hfp_connect_req(uint8_t bd_addr[6],
                     uint8_t remote_dlci,
                     bool    hfp_flag)
{
    uint8_t     profile_index;
    T_HFP_LINK *p_link;
    uint8_t     dlci;
    int32_t     ret = 0;

    if (hfp_find_link_by_addr(bd_addr) != NULL)
    {
        ret = 1;
        goto fail_link_existed;
    }

    p_link = hfp_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (hfp_flag)
    {
        profile_index = p_hfp->hfp_rfc_index;
    }
    else
    {
        profile_index = p_hfp->hsp_rfc_index;
    }

    if (rfc_conn_req(bd_addr, remote_dlci, RFC_DEFAULT_MTU, RFC_DEFAULT_CREDIT,
                     profile_index, &dlci) == false)
    {
        ret = 3;
        goto fail_rfc_conn;
    }

    p_link->dlci = dlci;
    return true;

fail_rfc_conn:
    hfp_free_link(p_link);
fail_alloc_link:
fail_link_existed:
    PROFILE_PRINT_ERROR2("hfp_connect_req: bd_addr %s, failed %d", TRACE_BDADDR(bd_addr), -ret);
    return false;
}

bool hfp_connect_cfm(uint8_t bd_addr[6],
                     bool    accept)
{
    T_HFP_LINK *p_link;
    uint16_t    status;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        status = accept ? RFC_ACCEPT : RFC_REJECT;

        rfc_conn_cfm(bd_addr, p_link->dlci, status, p_link->frame_size, RFC_DEFAULT_CREDIT);

        if (accept == false)
        {
            hfp_free_link(p_link);
        }

        return true;
    }

    return false;
}

bool hfp_disconnect_req(uint8_t bd_addr[6])
{
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        return rfc_disconn_req(p_link->bd_addr, p_link->dlci);
    }

    return false;
}

void hfp_tout_callback(T_SYS_TIMER_HANDLE handle)
{
    T_HFP_MSG_WAIT_RSP_TOUT message;
    T_HFP_LINK *p_link;
    T_HFP_CMD_ITEM *cmd_item;

    p_link = (void *)sys_timer_id_get(handle);
    if (p_link != NULL)
    {
        if (p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTED)
        {
            cmd_item = os_queue_out(&p_link->cmd_queue);
            if (cmd_item != NULL)
            {
                memcpy(message.bd_addr, p_link->bd_addr, 6);
                message.last_cmd_id = cmd_item->cmd_id;
                p_link->credits = 1;
                p_hfp->cback(p_link->bd_addr, HFP_MSG_ACK_TIMEOUT, &message);
                os_mem_free(cmd_item);
                hfp_flush_cmd(p_link);
            }
        }
        else if (p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTING)
        {
            hfp_srv_level_continue(p_link, false);
        }
    }
}

bool hfp_init(uint8_t        link_num,
              uint8_t        rfc_hfp_chann_num,
              uint8_t        rfc_hsp_chann_num,
              P_HFP_HF_CBACK cback,
              uint16_t       hfp_brsf_cpbs,
              uint16_t       voice_codec)
{
    int32_t ret = 0;

    p_hfp = (T_HFP *)os_mem_zalloc2(sizeof(T_HFP));
    if (p_hfp == NULL)
    {
        ret = 1;
        goto fail_alloc_hfp;
    }

    p_hfp->link_num = link_num;
    p_hfp->link_list = os_mem_zalloc2(p_hfp->link_num * sizeof(T_HFP_LINK));
    if (p_hfp->link_list == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    p_hfp->wait_rsp_tout = 3000; /* uint is ms */
    p_hfp->brsf_cpbs = hfp_brsf_cpbs;
    p_hfp->supt_voice_codec = voice_codec;
    p_hfp->cback = cback;

    if (rfc_reg_cb(rfc_hfp_chann_num, hfp_handle_rfc_msg, &p_hfp->hfp_rfc_index) == false)
    {
        ret = 3;
        goto fail_reg_hfp_rfc_cb;
    }

    if (rfc_reg_cb(rfc_hsp_chann_num, hfp_handle_rfc_msg, &p_hfp->hsp_rfc_index) == false)
    {
        ret = 4;
        goto fail_reg_hsp_rfc_cb;
    }

    return true;

fail_reg_hsp_rfc_cb:
fail_reg_hfp_rfc_cb:
    os_mem_free(p_hfp->link_list);
fail_alloc_link:
    os_mem_free(p_hfp);
    p_hfp = NULL;
fail_alloc_hfp:
    PROFILE_PRINT_ERROR1("hfp_init: failed %d", -ret);
    return false;
}

bool hfp_remote_capabilities_get(uint8_t   bd_addr[6],
                                 uint16_t *cpbs)
{
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        *cpbs = p_link->cpbs;
        return true;
    }
    return false;
}

uint16_t hfp_local_capabilities_get(void)
{
    return p_hfp->brsf_cpbs;
}

bool hfp_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_HFP_INFO *p_info)
{
    uint8_t count = 0;
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        p_info->cpbs = p_link->cpbs;
        p_info->uuid = p_link->uuid;
        p_info->at_cmd_credits = p_link->credits;
        p_info->rfc_dlci = p_link->dlci;
        p_info->state = p_link->hfp_state;
        p_info->frame_size = p_link->frame_size;
        p_info->indicator_cnt = p_link->indicator_map.size;

        for (count = 0; count < p_info->indicator_cnt; count++)
        {
            p_info->indicator_type[count] = p_link->indicator_map.indicator[count].type;
            p_info->indicator_range[count] = p_link->indicator_map.indicator[count].range;
        }

        return rfc_get_cid(p_link->bd_addr, p_info->rfc_dlci, &p_info->l2c_cid);
    }

    return false;
}

bool hfp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_HFP_INFO *p_info)
{
    uint8_t count = 0;
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        p_link = hfp_alloc_link(bd_addr);
    }

    if (p_link != NULL)
    {
        p_link->cpbs = p_info->cpbs;
        p_link->credits = p_info->at_cmd_credits;
        p_link->hfp_state = (T_HFP_STATE)p_info->state;
        p_link->uuid = p_info->uuid;
        p_link->dlci = p_info->rfc_dlci;
        p_link->frame_size = p_info->frame_size;

        if (p_link->credits == 0)
        {
            sys_timer_start(p_link->timer_handle);
        }

        p_link->indicator_map.size = p_info->indicator_cnt;
        for (count = 0; count < p_info->indicator_cnt; count++)
        {
            p_link->indicator_map.indicator[count].type = p_info->indicator_type[count];
            p_link->indicator_map.indicator[count].range = p_info->indicator_range[count];
        }

        return true;
    }

    return false;
}

bool hfp_del_roleswap_info(uint8_t bd_addr[6])
{
    T_HFP_LINK *p_link;

    p_link = hfp_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    hfp_free_link(p_link);

    return true;
}

uint8_t hfp_get_rfc_profile_idx(uint16_t uuid)
{
    if (uuid == UUID_HANDSFREE)
    {
        return p_hfp->hfp_rfc_index;
    }
    else
    {
        return p_hfp->hsp_rfc_index;
    }
}
#else
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "hfp.h"

bool hfp_init(uint8_t        link_num,
              uint8_t        rfc_hfp_chann_num,
              uint8_t        rfc_hsp_chann_num,
              P_HFP_HF_CBACK cback,
              uint16_t       hfp_brsf_cpbs,
              uint16_t       voice_codec)
{
    return false;
}

bool hfp_connect_req(uint8_t bd_addr[6],
                     uint8_t remote_dlci,
                     bool    hfp_flag)
{
    return false;
}

bool hfp_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_dial_with_number(uint8_t     bd_addr[6],
                          const char *number)
{
    return false;
}

bool hfp_dial_last_number(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_dtmf_send(uint8_t bd_addr[6],
                   char    c)
{
    return false;
}

bool hfp_accept_phone_call(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_reject_phone_call(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_set_voice_recognition(uint8_t bd_addr[6],
                               bool    enable)
{
    return false;
}

bool hfp_call_hold_action(uint8_t                bd_addr[6],
                          T_HFP_CALL_HOLD_ACTION control)
{
    return false;
}

bool hfp_3way_call_control_with_index(uint8_t bd_addr[6],
                                      uint8_t control,
                                      uint8_t index)
{
    return false;
}

bool hfp_inform_ag_microphone_gain(uint8_t bd_addr[6],
                                   uint8_t level)
{
    return false;
}

bool hfp_inform_ag_speaker_gain(uint8_t bd_addr[6],
                                uint8_t level)
{
    return false;
}

bool hfp_send_clcc(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_send_clip(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_send_nrec_disable(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_send_cmee_enable(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_send_ccwa(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_hsp_button_press(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_remote_capabilities_get(uint8_t   bd_addr[6],
                                 uint16_t *cpbs)
{
    return false;
}

bool hfp_ask_ag_establish_voice_chann(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_network_operator_format_set(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_network_operator_name_query(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_subscriber_num_query(uint8_t bd_addr[6])
{
    return false;
}

bool hfp_indicator_activate(uint8_t  bd_addr[6],
                            uint32_t indicator_types)
{
    return false;
}

bool hfp_indicator_deactivate(uint8_t  bd_addr[6],
                              uint32_t indicator_types)
{
    return false;
}

bool hfp_batt_level_report(uint8_t     bd_addr[6],
                           const char *at_cmd,
                           uint8_t     type)
{
    return false;
}

bool hfp_send_vnd_cmd(uint8_t     bd_addr[6],
                      const char *at_cmd)
{
    return false;
}

uint16_t hfp_local_capabilities_get(void)
{
    return 0;
}

bool hfp_connect_cfm(uint8_t bd_addr[6],
                     bool    accept)
{
    return false;
}

bool hfp_get_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_HFP_INFO *p_info)
{
    return false;
}

bool hfp_set_roleswap_info(uint8_t              bd_addr[6],
                           T_ROLESWAP_HFP_INFO *p_info)
{
    return false;
}

bool hfp_del_roleswap_info(uint8_t bd_addr[6])
{
    return false;
}

uint8_t hfp_get_rfc_profile_idx(uint16_t uuid)
{
    return false;
}
#endif
