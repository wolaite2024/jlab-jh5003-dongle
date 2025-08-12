/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HFP_AG_SUPPORT == 1)

#include <string.h>
#include <stdio.h>
#include "os_mem.h"
#include <stdlib.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "rfc.h"
#include "hfp_ag.h"

/*The Hands Free Profile specification limits the number of indicators returned by the AG to a maximum of 20.*/
#define MAX_AG_INDICATOR_COUNT 20

typedef struct t_hfp_ag_link
{
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
    uint32_t                hf_indicator_mask; /* determined by the hf BIND AT Cmd */
    uint32_t                ag_indicator_mask; /* determined by the hf BIA and CMER AT Cmd */
    uint32_t                ag_hf_xapl_mask; /* determined by the hf XAPL AT Cmd */
    bool                    cmer_enable; /* indication status update 1:enabled 0:disabled */
    bool                    cmee_enable; /* enhanced error code 1:enabled 0:disabled */
    bool                    clip_enable; /* calling line identification 1:enabled 0:disabled */
    bool                    ccwa_enable; /* calling waiting 1:enabled 0:disabled */
    bool                    cops_enable; /* long alphanumeric name format 1:enabled 0:disabled */
} T_HFP_AG_LINK;

typedef struct t_hfp_ag
{
    P_HFP_AG_CBACK          ag_cback;
    T_HFP_AG_LINK          *link_list;
    uint16_t                brsf_cpbs;
    uint8_t                 hfp_rfc_index; /* rfomm index, which will be used when connect */
    uint8_t                 hsp_rfc_index; /* rfomm index, which will be used when connect */
    uint8_t                 link_num;
    uint16_t                supt_voice_codec; /*bit0:CVSD;bit1:mSBC,bit2~15 vendor*/
} T_HFP_AG;

static T_HFP_AG *p_hfp_ag;

static const char hfp_ag_cind_info[] = "\r\n+CIND: (\"service\",(0-1)),(\"call\",(0,1)),"
                                       "(\"callsetup\",(0-3)),(\"callheld\",(0-2)),"
                                       "(\"signal\",(0-6)),(\"roam\",(0,1)),(\"battchg\",(0-5))\r\n";

T_HFP_AG_LINK *hfp_ag_alloc_link(uint8_t bd_addr[6])
{
    uint8_t        i;
    T_HFP_AG_LINK *p_link = NULL;

    for (i = 0; i < p_hfp_ag->link_num; i++)
    {
        if (p_hfp_ag->link_list[i].hfp_state == HFP_STATE_DISCONNECTED)
        {
            p_link = &p_hfp_ag->link_list[i];
            memcpy(p_link->bd_addr, bd_addr, 6);
            p_link->hfp_state = HFP_STATE_ALLOCATED;
            os_queue_init(&p_link->cmd_queue);
            break;
        }
    }

    return p_link;
}

void hfp_ag_free_link(T_HFP_AG_LINK *p_link)
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

        memset(p_link, 0, sizeof(T_HFP_AG_LINK));
    }
}

T_HFP_AG_LINK *hfp_ag_find_link_by_addr(uint8_t bd_addr[6])
{
    uint8_t        i;
    T_HFP_AG_LINK *p_link = NULL;

    for (i = 0; i < p_hfp_ag->link_num; i++)
    {
        if (!memcmp(p_hfp_ag->link_list[i].bd_addr, bd_addr, 6))
        {
            p_link = &p_hfp_ag->link_list[i];
            break;
        }
    }

    return p_link;
}

bool hfp_ag_flush_cmd(T_HFP_AG_LINK *p_link)
{
    T_HFP_CMD_ITEM *cmd_item;
    bool result;

    while ((p_link->cmd_queue.count > 0) && (p_link->credits > 0))
    {
        cmd_item = os_queue_out(&p_link->cmd_queue);
        if (cmd_item != NULL)
        {
            result = rfc_data_req(p_link->bd_addr,
                                  p_link->dlci,
                                  (uint8_t *)cmd_item->at_cmd,
                                  cmd_item->cmd_len,
                                  false);

            os_mem_free(cmd_item);

            if (result == true)
            {
                p_link->credits--;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

bool hfp_ag_try_general_at_cmd(uint8_t     bd_addr[6],
                               const char *at_cmd)
{
    T_HFP_AG_LINK  *p_link;
    int             cmd_len;
    T_HFP_CMD_ITEM *cmd_item;
    int32_t         ret = 0;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        ret = 1;
        goto fail_invalid_addr;
    }

    if (p_link->hfp_state < HFP_STATE_SRV_LEVEL_CONNECTING)
    {
        ret = 2;
        goto fail_invalid_state;
    }

    cmd_len = strlen(at_cmd);
    if ((cmd_len < 2) || (cmd_len > p_link->frame_size))
    {
        ret = 3;
        goto fail_invalid_cmd_length;
    }

    if ((at_cmd[cmd_len - 2] != '\r') && (at_cmd[cmd_len - 1] != '\n'))
    {
        ret = 4;
        goto fail_invalid_cmd_delimiter;
    }

    cmd_item = os_mem_zalloc2(sizeof(T_HFP_CMD_ITEM) + cmd_len);
    if (cmd_item == NULL)
    {
        ret = 5;
        goto fail_alloc_cmd_item;
    }

    memcpy((char *)cmd_item->at_cmd, at_cmd, cmd_len);
    cmd_item->cmd_len = cmd_len;

    os_queue_in(&p_link->cmd_queue, cmd_item);

    return hfp_ag_flush_cmd(p_link);

fail_alloc_cmd_item:
fail_invalid_cmd_delimiter:
fail_invalid_cmd_length:
fail_invalid_state:
fail_invalid_addr:
    PROFILE_PRINT_ERROR2("hfp_ag_try_general_at_cmd: bd_addr %s, failed %d",
                         TRACE_BDADDR(bd_addr), -ret);
    return false;
}

int32_t hfp_ag_handle_brsf_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res;
    int offset;
    char buf[17];
    int32_t ret = 0;

    res = sscanf(str, "AT+BRSF=%hu%n", &p_link->cpbs, &offset);
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

    snprintf(buf, 17, "\r\n+BRSF: %u\r\n", p_hfp_ag->brsf_cpbs);
    hfp_ag_try_general_at_cmd(p_link->bd_addr, buf);

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bac_set(T_HFP_AG_LINK *p_link,
                              const char    *str)
{
    int res;
    int offset;
    uint16_t codec_bitmap = 0;
    uint16_t codec_type;
    uint8_t codec_updated;
    int32_t  ret = 0;

    if (strncmp("AT+BAC=", str, 7))
    {
        ret = 1;
        goto fail_bac_cmd;
    }

    str += 7;
    while ((res = sscanf(str, "%hu%n", &codec_type, &offset)) == 1)
    {
        codec_bitmap |= (1 << (codec_type - 1));

        str += offset;
        if (*str != ',')
        {
            break;
        }

        str++;
    }

    if (res < 1)
    {
        ret = 2;
        goto fail_bac_set_value;
    }

    if (*str != '\0')
    {
        ret = 3;
        goto fail_bac_set_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    if (p_link->supt_voice_codec != codec_bitmap)
    {
        codec_updated = 1;
    }
    else
    {
        codec_updated = 0;
    }

    p_link->supt_voice_codec = codec_bitmap;
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_BAC_SET, &codec_updated);

    return 0;

fail_send_ok:
fail_bac_set_end_part:
fail_bac_set_value:
fail_bac_cmd:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cind_test(T_HFP_AG_LINK *p_link,
                                const char    *str)
{
    int32_t ret = 0;

    if (strncmp("AT+CIND=?", str, 9))
    {
        ret = 1;
        goto fail_cind_test_cmd;
    }

    hfp_ag_try_general_at_cmd(p_link->bd_addr, hfp_ag_cind_info);

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_cind_test_cmd:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cind_read(T_HFP_AG_LINK *p_link,
                                const char    *str)
{
    int32_t ret = 0;

    if (strncmp("AT+CIND?", str, 8))
    {
        ret = 1;
        goto fail_cind_read_cmd;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CIND, NULL);
    return 0;

fail_cind_read_cmd:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_nrec(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_NREC msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+NREC=%hhu%n", &msg.value, &offset);
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

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_NREC, &msg);
    return 0;

fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cmer_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res;
    int offset;
    uint8_t mode;
    uint8_t keypad;
    uint8_t display;
    uint8_t cmer_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CMER=%hhu,%hhu,%hhu,%hhu%n", &mode, &keypad, &display, &cmer_enable, &offset);
    if (res < 4)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (mode != 3 || keypad != 0 || display != 0)
    {
        ret = 2;
        goto fail_error_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 3;
        goto fail_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    p_link->cmer_enable = (cmer_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_error_value:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_chld_test(T_HFP_AG_LINK *p_link,
                                const char    *str)
{
    char buf[21];
    int32_t ret = 0;

    if (strncmp("AT+CHLD=?", str, 9))
    {
        ret = 1;
        goto fail_chld_chld_test;
    }

    snprintf(buf, 21, "\r\n+CHLD: %s\r\n", HFP_AG_CHLD_VAL);
    hfp_ag_try_general_at_cmd(p_link->bd_addr, buf);

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_chld_chld_test:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bind_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res;
    int offset;
    uint32_t hf_indicator_mask;
    int32_t ret = 0;

    if (strncmp("AT+BIND=", str, 8))
    {
        ret = 1;
        goto fail_bind_set_cmd;
    }

    str += 8;
    while ((res = sscanf(str, "%u%n", &hf_indicator_mask, &offset)) == 1)
    {
        p_link->hf_indicator_mask |= (1 << (hf_indicator_mask - 1));

        str += offset;
        if (*str != ',')
        {
            break;
        }

        str++;
    }

    if (res < 1)
    {
        ret = 2;
        goto fail_bind_set_value;
    }

    if (*str != '\0')
    {
        ret = 3;
        goto fail_bind_set_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    p_link->hf_indicator_mask &= HFP_HF_SUPPORTED_INDICATORS_MASK;

    return 0;

fail_send_ok:
fail_bind_set_end_part:
fail_bind_set_value:
fail_bind_set_cmd:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bind_test(T_HFP_AG_LINK *p_link,
                                const char    *str)
{
    char buf[17];
    uint8_t i;
    int offset = 10;
    int32_t ret = 0;

    if (strncmp("AT+BIND=?", str, 9))
    {
        ret = 1;
        goto fail_bind_test_cmd;
    }

    snprintf(buf, 11, "\r\n+BIND: (");

    if (HFP_HF_SUPPORTED_INDICATORS_MASK > 0)
    {
        for (i = 0; HFP_HF_SUPPORTED_INDICATORS_MASK >> i; i++)
        {
            if (HFP_HF_SUPPORTED_INDICATORS_MASK & (1 << i))
            {
                snprintf(buf + offset, 3, "%u,", i + 1);
                offset += 2;
            }
            else
            {
                snprintf(buf + offset, 2, ",");
                offset += 1;
            }
        }

        snprintf(buf + offset - 1, 4, ")\r\n");
    }
    else
    {
        snprintf(buf + offset, 4, ")\r\n");
    }

    hfp_ag_try_general_at_cmd(p_link->bd_addr, buf);

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_bind_test_cmd:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bind_read(T_HFP_AG_LINK *p_link,
                                const char    *str)
{
    uint8_t i;
    char buf[15];
    int32_t ret = 0;

    if (strncmp("AT+BIND?", str, 8))
    {
        ret = 1;
        goto fail_bind_read_cmd;
    }

    for (i = 0; HFP_HF_SUPPORTED_INDICATORS_MASK >> i; i++)
    {
        bool hf_ind_mask_check = p_link->hf_indicator_mask & (1 << i);
        if (hf_ind_mask_check)
        {
            snprintf(buf, 15, "\r\n+BIND: %u,%u\r\n", i + 1, hf_ind_mask_check);
            hfp_ag_try_general_at_cmd(p_link->bd_addr, buf);
        }
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 2;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_bind_read_cmd:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bcs_set(T_HFP_AG_LINK *p_link,
                              const char    *str)
{
    int res;
    int offset;
    uint8_t codec_type;
    T_HFP_AG_MSG_CODEC_TYPE_SELECTED msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+BCS=%hhu%n", &codec_type, &offset);
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

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    msg.codec_type = (T_HFP_CODEC_TYPE)codec_type;
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CODEC_TYPE_SELECT, &msg);
    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bcc(T_HFP_AG_LINK *p_link,
                          const char    *str)
{
    if (hfp_ag_send_ok(p_link->bd_addr) == true)
    {
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CODEC_NEGOTIATE, NULL);
        return 0;
    }

    return 1;
}

int32_t hfp_ag_handle_ata(T_HFP_AG_LINK *p_link,
                          const char    *str)
{
    if (hfp_ag_send_ok(p_link->bd_addr) == true)
    {
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_ACCEPT_CALL, NULL);
        return 0;
    }

    return 1;
}

int32_t hfp_ag_handle_chup(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    if (hfp_ag_send_ok(p_link->bd_addr) == true)
    {
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_REJECT_HANGUP_CALL, NULL);
        return 0;
    }

    return 1;
}

int32_t hfp_ag_handle_button_press(T_HFP_AG_LINK *p_link,
                                   const char    *str)
{
    if (hfp_ag_send_ok(p_link->bd_addr) == true)
    {
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_HSP_BUTTON_PRESS, NULL);
        return 0;
    }

    return 1;
}

int32_t hfp_ag_handle_vgs_set(T_HFP_AG_LINK *p_link,
                              const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_SPEAKER_VOLUME_CHANGED msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+VGS=%hhu%n", &msg.volume, &offset);
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

    if ((p_link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY) &&
        ((p_link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0))
    {
        ret = 3;
        goto fail_support_cmd;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_INFORM_SPEAKER_GAIN, &msg);
    return 0;

fail_send_ok:
fail_support_cmd:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_vgm_set(T_HFP_AG_LINK *p_link,
                              const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_MICROPHONE_VOLUME_CHANGED msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+VGM=%hhu%n", &msg.volume, &offset);
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

    if ((p_link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY) &&
        ((p_link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0))
    {
        ret = 3;
        goto fail_support_cmd;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_INFORM_MICROPHONE_GAIN, &msg);
    return 0;

fail_send_ok:
fail_support_cmd:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_xapl_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res = 0;
    int offset;
    char buf[20];
    int32_t ret = 0;

    res = sscanf(str, "AT+XAPL=%*4[^-]-%*4[^-]-%*4[^,],%u%n", &p_link->ag_hf_xapl_mask, &offset);
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

    snprintf(buf, 20, "\r\n+XAPL=iPhone,%u\r\n", HFP_HF_SUPPORTED_XAPL_FEATURES_MASK);
    p_link->ag_hf_xapl_mask &= HFP_HF_SUPPORTED_XAPL_FEATURES_MASK;
    if (hfp_ag_try_general_at_cmd(p_link->bd_addr, buf) == false)
    {
        ret = 3;
        goto fail_send_xapl;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_send_xapl:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_iphoneaccev_set(T_HFP_AG_LINK *p_link,
                                      const char    *str)
{
    int res;
    int offset;
    uint8_t i = 0;
    uint8_t xapl_num;
    T_HFP_AG_MSG_HF_XAPL msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+IPHONEACCEV=%hhu%n", &xapl_num, &offset);
    if (res < 0)
    {
        ret = 1;
        goto fail_iphoneaccev_num_get;
    }

    str += offset;
    while ((res = sscanf(str, ",%hhu,%hhu%n", &msg.xapl_id, &msg.xapl_value, &offset)) == 2)
    {
        memcpy(msg.bd_addr, p_link->bd_addr, 6);
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_IPHONEACCEV, &msg);

        i++;
        str += offset;
        if (*str != ',')
        {
            break;
        }
    }

    if (res < 2)
    {
        ret = 2;
        goto fail_iphoneaccev_value;
    }

    if (i != xapl_num)
    {
        ret = 3;
        goto fail_iphoneaccev_oversize;
    }

    if (*str != '\0')
    {
        ret = 4;
        goto fail_iphoneaccev_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 5;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_iphoneaccev_end_part:
fail_iphoneaccev_oversize:
fail_iphoneaccev_value:
fail_iphoneaccev_num_get:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_xevent_set(T_HFP_AG_LINK *p_link,
                                 const char    *str)
{
    int res;
    int offset;
    uint16_t level;
    uint16_t level_num;
    T_HFP_AG_MSG_HF_XEVENT msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+XEVENT=BATTERY,%hu,%hu,%*u,%*u%n", &level, &level_num, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (level_num == 0)
    {
        ret = 2;
        goto fail_invalid_level_num;
    }

    if (*(str + offset) != '\0')
    {
        ret = 3;
        goto fail_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    msg.battery_level = (level * 100) / level_num;
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_XEVENT, &msg);
    return 0;

fail_send_ok:
fail_end_part:
fail_invalid_level_num:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_biev_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_HF_IND msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+BIEV=%hu,%u%n", &msg.ind_id, &msg.ind_value, &offset);
    if (res < 2)
    {
        ret = 1;
        goto fail_no_value;
    }

    if (msg.ind_id > HFP_HF_IND_BATTERY_LEVEL_STATUS_ID)
    {
        ret = 2;
        goto fail_invalid_id;
    }

    if (*(str + offset) != '\0')
    {
        ret = 3;
        goto fail_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 4;
        goto fail_send_ok;
    }

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_BIEV, &msg);
    return 0;

fail_send_ok:
fail_end_part:
fail_invalid_id:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bia_set(T_HFP_AG_LINK *p_link,
                              const char    *str)
{
    int res;
    int offset = 0;
    uint8_t i = 0;
    uint8_t value;
    int32_t ret = 0;

    str += 7;
    while ((res = sscanf(str, "%hhu%n", &value, &offset)) >= 0)
    {
        if (res >= 0)
        {
            if (res == 1)
            {
                if (value == 1)
                {
                    p_link->ag_indicator_mask |= (1 << i);
                }
                else if (value == 0)
                {
                    p_link->ag_indicator_mask &= ~(1 << i);
                }
                else
                {
                    ret = 1;
                    goto fail_bia_invalid_value;
                }
            }

            i++;

            str += offset;
            offset = 0;
            if (*str != ',')
            {
                break;
            }

            str++;
        }
    }

    if (res < 1)
    {
        ret = 2;
        goto fail_bia_value_get;
    }

    if (i > MAX_AG_INDICATOR_COUNT)
    {
        ret = 3;
        goto fail_bia_oversize;
    }

    if (*str != '\0')
    {
        ret = 4;
        goto fail_bia_set_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 5;
        goto fail_send_ok;
    }

    return 0;

fail_send_ok:
fail_bia_set_end_part:
fail_bia_oversize:
fail_bia_value_get:
fail_bia_invalid_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cmee_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res;
    int offset;
    uint8_t cmee_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CMEE=%hhu%n", &cmee_enable, &offset);
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

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    p_link->cmee_enable = (cmee_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_clip_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int res;
    int offset;
    uint8_t clip_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CLIP=%hhu%n", &clip_enable, &offset);
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

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    p_link->clip_enable = (clip_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_clcc(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    if (*(str + 7) != '\0')
    {
        hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
        return 1;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CLCC, NULL);
    return 0;
}

int32_t hfp_ag_handle_atd(T_HFP_AG_LINK *p_link,
                          const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_DIAL_WITH_NUMBER msg;
    int32_t ret = 0;

    memset(msg.number, 0, sizeof(msg.number));
    memcpy(msg.bd_addr, p_link->bd_addr, 6);

    res = sscanf(str, "ATD%20[^;];%n", msg.number, &offset);
    if ((res < 1) || (offset <= 0))
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_DIAL_WITH_NUMBER, &msg);
    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_ext_atd(T_HFP_AG_LINK *p_link,
                              const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_DIAL_WITH_MEMORY msg;
    int32_t ret = 0;

    res = sscanf(str, "ATD>%hhu;%n", &msg.num, &offset);
    if ((res < 1) || (offset <= 0))
    {
        ret = 1;
        goto fail_no_value;
    }

    if (*(str + offset) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_DIAL_WITH_MEMORY, &msg);
    return 0;

fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bldn(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    int32_t ret = 0;

    if (*(str + 7) != '\0')
    {
        ret = 1;
        goto fail_end_part;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_DIAL_LAST_NUMBER, NULL);
    return 0;

fail_end_part:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_chld(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_CHLD msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+CHLD=%hhu%n", &msg.value, &offset);
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

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CHLD, &msg);
    return 0;

fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_ccwa(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    int res;
    int offset;
    uint8_t ccwa_enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+CCWA=%hhu%n", &ccwa_enable, &offset);
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

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    p_link->ccwa_enable = (ccwa_enable == 1);

    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cnum(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    if (*(str + 7) != '\0')
    {
        hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
        return 1;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CNUM, NULL);
    return 0;
}

int32_t hfp_ag_handle_vts(T_HFP_AG_LINK *p_link,
                          const char    *str)
{
    int res;
    int offset;
    T_HFP_AG_MSG_CALL_DTMF_CODE msg;
    int32_t ret = 0;

    res = sscanf(str, "AT+VTS=%c%n", &msg.dtmf_code, &offset);
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

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    memcpy(msg.bd_addr, p_link->bd_addr, 6);
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_VTS, &msg);
    return 0;

fail_send_ok:
fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cops_set(T_HFP_AG_LINK *p_link,
                               const char    *str)
{
    int32_t ret = 0;

    if (strncmp("AT+COPS=3,0", str, 11))
    {
        ret = 1;
        goto fail_value_get;
    }

    if (*(str + 11) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    if (hfp_ag_send_ok(p_link->bd_addr) == false)
    {
        ret = 3;
        goto fail_send_ok;
    }

    p_link->cops_enable = true;
    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_COPS_SET, NULL);
    return 0;

fail_send_ok:
fail_end_part:
fail_value_get:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_cops_read(T_HFP_AG_LINK *p_link,
                                const char    *str)
{
    int32_t ret = 0;

    if (p_link->cops_enable == false)
    {
        ret = 1;
        goto fail_cops_enable;
    }

    if (*(str + 8) != '\0')
    {
        ret = 2;
        goto fail_end_part;
    }

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_COPS_QUERY, NULL);
    return 0;

fail_end_part:
fail_cops_enable:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

int32_t hfp_ag_handle_bvra(T_HFP_AG_LINK *p_link,
                           const char    *str)
{
    int res;
    int offset;
    uint8_t enable;
    int32_t ret = 0;

    res = sscanf(str, "AT+BVRA=%hhu%n", &enable, &offset);
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

    if (enable == 1)
    {
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_ACTIVATE_VOICE_RECOGNITION, NULL);
    }
    else if (enable == 0)
    {
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_DEACTIVATE_VOICE_RECOGNITION, NULL);
    }

    return ret;

fail_end_part:
fail_no_value:
    hfp_ag_send_error(p_link->bd_addr, HFP_AG_ERR_INV_CHAR_IN_TSTR);
    return ret;
}

void hfp_ag_srv_level_established(T_HFP_AG_LINK *p_link)
{
    T_HFP_AG_MSG_CONN msg;

    p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTED;
    memcpy(msg.bd_addr, p_link->bd_addr, 6);

    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_CONN, &msg);
    /* now all init commands are processed -> service level connection is established */
}

void hfp_ag_srv_level_step(T_HFP_AG_LINK *p_link,
                           const char    *str,
                           bool           result_ok)
{
    int32_t ret = 0;

    switch (p_link->service_level_step)
    {
    case HFP_SRV_LEVEL_NONE:
        {
            memset(&p_link->cmd_queue, 0, sizeof(p_link->cmd_queue));
            p_link->service_level_step = HFP_SRV_LEVEL_BRSF;
        }
        break;

    case HFP_SRV_LEVEL_BRSF: //M
        {
            ret = hfp_ag_handle_brsf_set(p_link, str);
            if (ret == 0)
            {
                if ((p_link->cpbs & HF_CAPABILITY_CODEC_NEGOTIATION) &&
                    (p_hfp_ag->brsf_cpbs & AG_CAPABILITY_CODEC_NEGOTIATION))
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_BAC;
                }
                else
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_CIND_TEST;
                }
            }
        }
        break;

    case HFP_SRV_LEVEL_BAC: //O
        {
            ret = hfp_ag_handle_bac_set(p_link, str);
            if (ret == 0)
            {
                p_link->service_level_step = HFP_SRV_LEVEL_CIND_TEST;
            }
        }
        break;

    case HFP_SRV_LEVEL_CIND_TEST: //M
        {
            ret = hfp_ag_handle_cind_test(p_link, str);
            if (ret == 0)
            {
                p_link->service_level_step = HFP_SRV_LEVEL_CIND_INQUIRY;
            }
        }
        break;

    case HFP_SRV_LEVEL_CIND_INQUIRY: //M
        /*The AT+CIND? read command is used to get current status of the AG indicators.*/
        {
            ret = hfp_ag_handle_cind_read(p_link, str);
            if (ret == 0)
            {
                p_link->service_level_step = HFP_SRV_LEVEL_ACTIVATE_INDICATOR;
            }
        }
        break;

    case HFP_SRV_LEVEL_ACTIVATE_INDICATOR: //M
        /*  AT+CMER=3,0,0,1 activates indicator events reporting.
            AT+CMER=3,0,0,0 deactivates indicator events reporting.*/
        {
            ret = hfp_ag_handle_cmer_set(p_link, str);
            if (ret == 0)
            {
                if ((p_link->cpbs & HF_CAPABILITY_CALL_WAITING_OR_3WAY) &&
                    (p_hfp_ag->brsf_cpbs & AG_CAPABILITY_3WAY))
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_CHLD_TEST;
                }
                else if ((p_link->cpbs & HF_CAPABILITY_HF_INDICATORS) &&
                         (p_hfp_ag->brsf_cpbs & AG_CAPABILITY_HF_INDICATORS))
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_BIND;
                }
                else
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_ESTABLISHED;
                    hfp_ag_srv_level_established(p_link);
                }
            }
        }
        break;

    case HFP_SRV_LEVEL_CHLD_TEST: //O, after this ServiceLevelConnection is Established
        {
            /* AT+CHLD=? get call hold capabilities */
            ret = hfp_ag_handle_chld_test(p_link, str);
            if (ret == 0)
            {
                if ((p_link->cpbs & HF_CAPABILITY_HF_INDICATORS) &&
                    (p_hfp_ag->brsf_cpbs & AG_CAPABILITY_HF_INDICATORS))
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_BIND;
                }
                else
                {
                    p_link->service_level_step = HFP_SRV_LEVEL_ESTABLISHED;
                    hfp_ag_srv_level_established(p_link);
                }
            }
        }
        break;

    case HFP_SRV_LEVEL_BIND:
        {
            ret = hfp_ag_handle_bind_set(p_link, str);
            if (ret == 0)
            {
                p_link->service_level_step = HFP_SRV_LEVEL_BIND_TEST;
            }
        }
        break;

    case HFP_SRV_LEVEL_BIND_TEST:
        {
            ret = hfp_ag_handle_bind_test(p_link, str);
            if (ret == 0)
            {
                p_link->service_level_step = HFP_SRV_LEVEL_BIND_INQUIRY;
            }
        }
        break;

    case HFP_SRV_LEVEL_BIND_INQUIRY:
        {
            ret = hfp_ag_handle_bind_read(p_link, str);
            if (ret == 0)
            {
                p_link->service_level_step = HFP_SRV_LEVEL_ESTABLISHED;
                hfp_ag_srv_level_established(p_link);
            }
        }
        break;

    default:
        break;
    }

    if (ret != 0)
    {
        PROFILE_PRINT_ERROR2("hfp_ag_srv_level_step: step %d, failed %d", p_link->service_level_step, -ret);
    }
}

void hfp_ag_srv_level_continue(T_HFP_AG_LINK *p_link,
                               const char    *str,
                               bool           result_ok)
{
    if (p_link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY)
    {
        hfp_ag_srv_level_step(p_link, str, result_ok);
    }
    else //for headset, no service level
    {
        hfp_ag_srv_level_established(p_link);
    }
}

void hfp_ag_srv_level_start(T_HFP_AG_LINK *p_link)
{
    p_link->service_level_step = HFP_SRV_LEVEL_NONE;

    hfp_ag_srv_level_continue(p_link, NULL, true);
}

bool hfp_ag_send_ok(uint8_t bd_addr[6])
{
    char buf[7];

    snprintf(buf, 7, "\r\nOK\r\n");

    return hfp_ag_try_general_at_cmd(bd_addr, buf);
}

bool hfp_ag_send_error(uint8_t bd_addr[6],
                       uint8_t error_code)
{
    char buf[19];
    T_HFP_AG_LINK *p_link;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        return false;
    }

    if (p_link->cmee_enable)
    {
        snprintf(buf, 19, "\r\n+CME ERROR: %u\r\n", error_code);
    }
    else
    {
        snprintf(buf, 10, "\r\nERROR\r\n");
    }

    return hfp_ag_try_general_at_cmd(bd_addr, buf);
}

void hfp_ag_handle_at_cmd(T_HFP_AG_LINK *p_link,
                          const char    *hf_at_cmd)
{
    char    *at_cmd;
    int32_t  ret = 0;

    if (p_link->hfp_state == HFP_STATE_SRV_LEVEL_CONNECTING)
    {
        hfp_ag_srv_level_continue(p_link, hf_at_cmd, true);
    }
    else
    {
        if (!strncmp("AT+BCS=", hf_at_cmd, 7))
        {
            at_cmd = "AT+BCS=";
            ret = hfp_ag_handle_bcs_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+BAC=", hf_at_cmd, 7))
        {
            at_cmd = "AT+BAC=";
            ret = hfp_ag_handle_bac_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+BCC", hf_at_cmd, 6))
        {
            at_cmd = "AT+BCC=";
            ret = hfp_ag_handle_bcc(p_link, hf_at_cmd);
        }
        else if (!strncmp("ATA", hf_at_cmd, 3))
        {
            at_cmd = "ATA";
            ret = hfp_ag_handle_ata(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CHUP", hf_at_cmd, 7))
        {
            at_cmd = "AT+CHUP=";
            ret = hfp_ag_handle_chup(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CKPD=200", hf_at_cmd, 11))
        {
            at_cmd = "AT+CKPD=200";
            ret = hfp_ag_handle_button_press(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+VGS=", hf_at_cmd, 7))
        {
            at_cmd = "AT+VGS=";
            ret = hfp_ag_handle_vgs_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+VGM=", hf_at_cmd, 7))
        {
            at_cmd = "AT+VGM=";
            ret = hfp_ag_handle_vgm_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+XAPL=", hf_at_cmd, 8))
        {
            at_cmd = "AT+XAPL=";
            ret = hfp_ag_handle_xapl_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+IPHONEACCEV=", hf_at_cmd, 15))
        {
            at_cmd = "AT+IPHONEACCEV=";
            ret = hfp_ag_handle_iphoneaccev_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+XEVENT=", hf_at_cmd, 10))
        {
            at_cmd = "AT+XEVENT=";
            ret = hfp_ag_handle_xevent_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+BIEV=", hf_at_cmd, 8))
        {
            at_cmd = "AT+BIEV=";
            ret = hfp_ag_handle_biev_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+BIA=", hf_at_cmd, 7))
        {
            at_cmd = "AT+BIA=";
            ret = hfp_ag_handle_bia_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CMEE=", hf_at_cmd, 8))
        {
            at_cmd = "AT+CMEE=";
            ret = hfp_ag_handle_cmee_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CLIP=", hf_at_cmd, 8))
        {
            at_cmd = "AT+CLIP=";
            ret = hfp_ag_handle_clip_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CLCC", hf_at_cmd, 7))
        {
            at_cmd = "AT+CLCC";
            ret = hfp_ag_handle_clcc(p_link, hf_at_cmd);
        }
        else if (!strncmp("ATD>", hf_at_cmd, 4))
        {
            at_cmd = "ATD>";
            ret = hfp_ag_handle_ext_atd(p_link, hf_at_cmd);
        }
        else if (!strncmp("ATD", hf_at_cmd, 3))
        {
            at_cmd = "ATD";
            ret = hfp_ag_handle_atd(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+BLDN", hf_at_cmd, 7))
        {
            at_cmd = "AT+BLDN";
            ret = hfp_ag_handle_bldn(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CHLD", hf_at_cmd, 7))
        {
            at_cmd = "AT+CHLD";
            ret = hfp_ag_handle_chld(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CCWA", hf_at_cmd, 7))
        {
            at_cmd = "AT+CCWA";
            ret = hfp_ag_handle_ccwa(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CNUM", hf_at_cmd, 7))
        {
            at_cmd = "AT+CNUM";
            ret = hfp_ag_handle_cnum(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+COPS=", hf_at_cmd, 8))
        {
            at_cmd = "AT+COPS=";
            ret = hfp_ag_handle_cops_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+COPS?", hf_at_cmd, 8))
        {
            at_cmd = "AT+COPS?";
            ret = hfp_ag_handle_cops_read(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+VTS=", hf_at_cmd, 7))
        {
            at_cmd = "AT+VTS=";
            ret = hfp_ag_handle_vts(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CMER=", hf_at_cmd, 8))
        {
            at_cmd = "AT+CMER=";
            ret = hfp_ag_handle_cmer_set(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+CIND?", hf_at_cmd, 8))
        {
            at_cmd = "AT+CIND?";
            ret = hfp_ag_handle_cind_read(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+NREC=", hf_at_cmd, 8))
        {
            at_cmd = "AT+NREC=";
            ret = hfp_ag_handle_nrec(p_link, hf_at_cmd);
        }
        else if (!strncmp("AT+BVRA", hf_at_cmd, 7))
        {
            at_cmd = "AT+BVRA";
            ret = hfp_ag_handle_bvra(p_link, hf_at_cmd);
        }
        else /* unkown AT command*/
        {
            p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_UNKNOWN_CMD, (void *)hf_at_cmd);
        }

        if (ret != 0)
        {
            PROFILE_PRINT_ERROR2("hfp_ag_handle_at_cmd: %s, failed %d", TRACE_STRING(at_cmd), -ret);
        }
    }
}

bool hfp_ag_codec_negotiate(uint8_t  bd_addr[6],
                            bool     fallback,
                            uint8_t *codec_type_sent)
{
    T_HFP_AG_LINK *p_link;
    uint8_t codec_type = CODEC_TYPE_CVSD;
    char buf[14];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (fallback == false)
        {
            uint8_t codec_bitmap;

            codec_bitmap = p_link->supt_voice_codec & p_hfp_ag->supt_voice_codec;

            if (codec_bitmap & (1 << (CODEC_TYPE_LC3 - 1)))
            {
                codec_type = CODEC_TYPE_LC3;
            }
            else if (codec_bitmap & (1 << (CODEC_TYPE_MSBC - 1)))
            {
                codec_type = CODEC_TYPE_MSBC;
            }
        }

        *codec_type_sent = codec_type;
        snprintf(buf, 14, "\r\n+BCS: %u\r\n", codec_type);

        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}

bool hfp_ag_send_cind(uint8_t                 bd_addr[6],
                      T_HFP_SERVICE_STATUS    service_status,
                      T_HFP_CALL_STATUS       call_status,
                      T_HFP_CALL_SETUP_STATUS call_setup_status,
                      T_HFP_CALL_HELD_STATUS  call_held_status,
                      uint8_t                 signal_status,
                      T_HFP_ROAM_STATUS       roam_status,
                      uint8_t                 batt_chg_status)
{
    char buf[25];

    snprintf(buf, 25, "\r\n+CIND: %u,%u,%u,%u,%u,%u,%u\r\n",
             service_status,
             call_status,
             call_setup_status,
             call_held_status,
             signal_status,
             roam_status,
             batt_chg_status);

    return hfp_ag_try_general_at_cmd(bd_addr, buf);
}

bool hfp_ag_send_ciev(uint8_t bd_addr[6],
                      uint8_t ind_id,
                      uint8_t ind_value)
{
    T_HFP_AG_LINK *p_link;
    char buf[15];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->cmer_enable)
        {
            if ((ind_id == AG_INDICATOR_TYPE_CALL) ||
                (ind_id == AG_INDICATOR_TYPE_CALLSETUP) ||
                (ind_id == AG_INDICATOR_TYPE_CALLHELD))
            {
                snprintf(buf, 15, "\r\n+CIEV: %u,%u\r\n", ind_id + 1, ind_value);
                return hfp_ag_try_general_at_cmd(bd_addr, buf);
            }
            else
            {
                if (p_link->ag_indicator_mask & (1 << ind_id))
                {
                    snprintf(buf, 15, "\r\n+CIEV: %u,%u\r\n", ind_id + 1, ind_value);
                    return hfp_ag_try_general_at_cmd(bd_addr, buf);
                }
            }
        }
    }

    return false;
}

bool hfp_ag_send_ring(uint8_t bd_addr[6])
{
    char buf[9];

    snprintf(buf, 9, "\r\nRING\r\n");

    return hfp_ag_try_general_at_cmd(bd_addr, buf);
}

bool hfp_ag_send_clip(uint8_t     bd_addr[6],
                      const char *call_num,
                      uint8_t     call_num_type)
{
    T_HFP_AG_LINK *p_link;
    char buf[37];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->clip_enable)
        {
            snprintf(buf, 37, "\r\n+CLIP: \"%s\",%u\r\n", call_num, call_num_type);
            return hfp_ag_try_general_at_cmd(bd_addr, buf);
        }
    }

    return false;
}

bool hfp_ag_send_vgm(uint8_t bd_addr[6],
                     uint8_t level)
{
    T_HFP_AG_LINK *p_link;
    char buf[13];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY)
        {
            if ((p_link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
            {
                return false;
            }

            snprintf(buf, 13, "\r\n+VGM: %u\r\n", level);
        }
        else
        {
            snprintf(buf, 12, "\r\n+VGM=%u\r\n", level);
        }

        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}


bool hfp_ag_send_vgs(uint8_t bd_addr[6],
                     uint8_t level)
{
    T_HFP_AG_LINK *p_link;
    char buf[13];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->uuid == UUID_HANDSFREE_AUDIO_GATEWAY)
        {
            if ((p_link->cpbs & HF_CAPABILITY_REMOTE_VOLUME_CONTROL) == 0)
            {
                return false;
            }

            snprintf(buf, 13, "\r\n+VGS: %u\r\n", level);
        }
        else
        {
            snprintf(buf, 12, "\r\n+VGS=%u\r\n", level);
        }

        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}

bool hfp_ag_send_ccwa(uint8_t     bd_addr[6],
                      const char *call_num,
                      uint8_t     call_num_type)
{
    T_HFP_AG_LINK *p_link;
    char buf[37];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->ccwa_enable)
        {
            snprintf(buf, 37, "\r\n+CCWA: \"%s\",%u\r\n", call_num, call_num_type);
            return hfp_ag_try_general_at_cmd(bd_addr, buf);
        }
    }

    return false;
}

bool hfp_ag_send_cops(uint8_t     bd_addr[6],
                      const char *operator_name)
{
    T_HFP_AG_LINK *p_link;
    char buf[35];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (p_link->cops_enable)
        {
            snprintf(buf, 35, "\r\n+COPS: ,3,0,\"%s\"\r\n", operator_name);
            return hfp_ag_try_general_at_cmd(bd_addr, buf);
        }
    }

    return false;
}

bool hfp_ag_send_cnum(uint8_t     bd_addr[6],
                      const char *call_num,
                      uint8_t     call_num_type,
                      uint8_t     service)
{
    T_HFP_AG_LINK *p_link;
    char buf[41];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        snprintf(buf, 41, "\r\n+CNUM: ,\"%s\",%u,,%u\r\n", call_num, call_num_type, service);
        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}

bool hfp_ag_inband_ringing_set(uint8_t bd_addr[6],
                               bool    enable)
{
    T_HFP_AG_LINK *p_link;
    char           buf[13];
    uint8_t        value;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (enable)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }

        snprintf(buf, 13, "\r\n+BSIR: %u\r\n", value);

        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}

bool hfp_ag_current_calls_list_send(uint8_t     bd_addr[6],
                                    uint8_t     call_idx,
                                    uint8_t     call_dir,
                                    uint8_t     call_status,
                                    uint8_t     call_mode,
                                    uint8_t     mpty,
                                    const char *call_num,
                                    uint8_t     call_num_type)
{
    T_HFP_AG_LINK *p_link;
    char buf[45];

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (call_num != NULL)
        {
            snprintf(buf, 45, "\r\n+CLCC: %u,%u,%u,%u,%u,%s,%u\r\n",
                     call_idx,
                     call_dir,
                     call_status,
                     call_mode,
                     mpty,
                     call_num,
                     call_num_type);
        }
        else
        {
            snprintf(buf, 45, "\r\n+CLCC: %u,%u,%u,%u,%u\r\n",
                     call_idx,
                     call_dir,
                     call_status,
                     call_mode,
                     mpty);
        }

        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}

bool hfp_ag_send_bvra(uint8_t bd_addr[6],
                      bool    enable)
{
    T_HFP_AG_LINK *p_link;
    char           buf[13];
    uint8_t        value;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (enable)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }

        snprintf(buf, 13, "\r\n+BVRA: %u\r\n", value);

        return hfp_ag_try_general_at_cmd(bd_addr, buf);
    }

    return false;
}

void hfp_ag_handle_rfc_conn_cmpl(uint8_t  bd_addr[6],
                                 uint8_t  index,
                                 uint8_t  dlci,
                                 uint8_t  credits,
                                 uint16_t frame_size)
{
    T_HFP_AG_LINK *p_link;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link == NULL)
    {
        rfc_disconn_req(bd_addr, dlci);
        return;
    }

    if (index == p_hfp_ag->hfp_rfc_index)
    {
        p_link->uuid = UUID_HANDSFREE_AUDIO_GATEWAY;
    }
    else if (index == p_hfp_ag->hsp_rfc_index)
    {
        p_link->uuid = UUID_HEADSET_AUDIO_GATEWAY;
    }
    else
    {
        PROFILE_PRINT_ERROR1("hfp_ag_handle_rfc_conn_cmpl: wrong index[%d] received", index);
        return;
    }

    p_link->frame_size = frame_size;
    {
        T_HFP_AG_MSG_RFC_CONN msg;
        memcpy(msg.bd_addr, p_link->bd_addr, 6);
        msg.hsp = p_link->uuid == UUID_HEADSET_AUDIO_GATEWAY ? true : false;
        p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_RFC_CONN, &msg);
    }

    if (credits == 0) //when connected credits may = 0, wait till credits > 0
    {
        p_link->hfp_state = HFP_STATE_RFC_CONN_NO_CREDITS;
    }
    else
    {
        p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
        p_link->credits = credits;
        hfp_ag_srv_level_start(p_link);
    }

    rfc_data_cfm(bd_addr, dlci, RFC_DEFAULT_CREDIT);
}

void hfp_ag_handle_rfc_data_ind(T_RFC_DATA_IND *p_data_msg)
{
    uint8_t       *p_data;
    uint8_t       *temp_ptr = NULL;
    uint16_t       length;
    T_HFP_AG_LINK *p_link;

    p_link = hfp_ag_find_link_by_addr(p_data_msg->bd_addr);
    if (p_link == NULL)
    {
        return;
    }

    p_link->credits = p_data_msg->remain_credits;
    if (p_link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && p_data_msg->remain_credits > 0)
    {
        p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
        hfp_ag_srv_level_start(p_link);
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
            hfp_ag_handle_at_cmd(p_link, (const char *)temp_ptr);
            p_link->rx_len = 0; //clear buffer
        }
        p_data++;
        length--;
    }

    p_data = p_link->rx_ptr;
    p_link->rx_ptr = NULL;
    if (p_link->rx_len) //means remainder exist
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

    //rsp_and_return:
    rfc_data_cfm(p_data_msg->bd_addr, p_data_msg->dlci, 1);

    return;
}

void hfp_ag_handle_rfc_msg(T_RFC_MSG_TYPE  msg_type,
                           void           *p_msg)
{
    switch (msg_type)
    {
    case RFC_CONN_IND:
        {
            T_RFC_CONN_IND *p_conn_ind_msg;
            T_HFP_AG_LINK  *p_link;

            p_conn_ind_msg = (T_RFC_CONN_IND *)p_msg;
            p_link = hfp_ag_find_link_by_addr(p_conn_ind_msg->bd_addr);
            if (p_link == NULL)
            {
                p_link = hfp_ag_alloc_link(p_conn_ind_msg->bd_addr);
                if (p_link == NULL)
                {
                    rfc_conn_cfm(p_conn_ind_msg->bd_addr, p_conn_ind_msg->dlci, RFC_REJECT,
                                 p_conn_ind_msg->frame_size, RFC_DEFAULT_CREDIT);
                }
                else
                {
                    p_link->dlci = p_conn_ind_msg->dlci;
                    p_link->ag_indicator_mask = HFP_AG_IND_ALL_MASK;
                    p_link->ag_hf_xapl_mask = HFP_HF_SUPPORTED_XAPL_FEATURES_MASK;
                    p_link->frame_size = p_conn_ind_msg->frame_size;
                    p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_RFC_CONN_IND, p_conn_ind_msg->bd_addr);
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
            T_RFC_CONN_CMPL *p_complete_msg;

            p_complete_msg = (T_RFC_CONN_CMPL *)p_msg;
            hfp_ag_handle_rfc_conn_cmpl(p_complete_msg->bd_addr,
                                        p_complete_msg->profile_index,
                                        p_complete_msg->dlci,
                                        p_complete_msg->remain_credits,
                                        p_complete_msg->frame_size);
        }
        break;

    case RFC_DISCONN_CMPL:
        {
            T_RFC_DISCONN_CMPL *p_disc_msg;
            T_HFP_AG_LINK      *p_link;

            p_disc_msg = (T_RFC_DISCONN_CMPL *)p_msg;
            p_link = hfp_ag_find_link_by_addr(p_disc_msg->bd_addr);
            if (p_link != NULL)
            {
                T_HFP_AG_DISCONN_INFO info;
                info.cause = p_disc_msg->cause;
                p_hfp_ag->ag_cback(p_link->bd_addr, HFP_AG_MSG_DISCONN, &info);
                hfp_ag_free_link(p_link);
            }
        }
        break;

    case RFC_CREDIT_INFO:
        {
            T_RFC_CREDIT_INFO *p_credit_msg;
            T_HFP_AG_LINK     *p_link;

            p_credit_msg = (T_RFC_CREDIT_INFO *)p_msg;
            p_link = hfp_ag_find_link_by_addr(p_credit_msg->bd_addr);
            if (p_link != NULL)
            {
                p_link->credits = p_credit_msg->remain_credits;
                if (p_link->hfp_state == HFP_STATE_RFC_CONN_NO_CREDITS && p_credit_msg->remain_credits > 0)
                {
                    p_link->hfp_state = HFP_STATE_SRV_LEVEL_CONNECTING;
                    hfp_ag_srv_level_start(p_link);
                }
                else if (p_link->hfp_state >= HFP_STATE_SRV_LEVEL_CONNECTING && p_credit_msg->remain_credits > 0)
                {
                    hfp_ag_flush_cmd(p_link);
                }
            }
        }
        break;

    case RFC_DATA_IND:
        hfp_ag_handle_rfc_data_ind((T_RFC_DATA_IND *)p_msg);
        break;

    default:
        break;
    }
}

bool hfp_ag_connect_req(uint8_t bd_addr[6],
                        uint8_t remote_dlci,
                        uint8_t hfp_ag_flag)
{
    uint8_t        profile_index;
    T_HFP_AG_LINK *p_link;
    uint8_t        dlci;
    int32_t        ret = 0;

    if (hfp_ag_find_link_by_addr(bd_addr) != NULL)//check database is available or not
    {
        ret = 1;
        goto fail_link_existing;
    }

    p_link = hfp_ag_alloc_link(bd_addr);
    if (p_link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    if (hfp_ag_flag)
    {
        profile_index = p_hfp_ag->hfp_rfc_index;
    }
    else
    {
        profile_index = p_hfp_ag->hsp_rfc_index;
    }

    if (rfc_conn_req(bd_addr, remote_dlci, RFC_DEFAULT_MTU, 0, profile_index, &dlci) == false)
    {
        ret = 3;
        goto fail_link_no_free_database;
    }

    p_link->dlci = dlci;
    p_link->ag_indicator_mask = HFP_AG_IND_ALL_MASK;
    p_link->ag_hf_xapl_mask = HFP_HF_SUPPORTED_XAPL_FEATURES_MASK;

    return true;

fail_alloc_link:
fail_link_existing:
fail_link_no_free_database:
    PROFILE_PRINT_ERROR2("hfp_ag_connect_req: bd_addr %s, failed %d", TRACE_BDADDR(bd_addr), -ret);
    return false;
}

bool hfp_ag_connect_cfm(uint8_t bd_addr[6],
                        bool    accept)
{
    T_HFP_AG_LINK *p_link;
    uint16_t       status;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        status = accept ? RFC_ACCEPT : RFC_REJECT;

        rfc_conn_cfm(bd_addr, p_link->dlci, status, p_link->frame_size, 0);

        if (accept == false)
        {
            hfp_ag_free_link(p_link);
        }

        return true;
    }

    return false;
}

bool hfp_ag_disconnect_req(uint8_t bd_addr[6])
{
    T_HFP_AG_LINK *p_link;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        if (rfc_disconn_req(p_link->bd_addr, p_link->dlci) == false)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    return false;
}

bool hfp_ag_init(uint8_t        link_num,
                 uint8_t        rfc_hfp_chann_num,
                 uint8_t        rfc_hsp_chann_num,
                 P_HFP_AG_CBACK cback,
                 uint16_t       hfp_ag_brsf_cpbs,
                 uint16_t       voice_codec)
{
    int32_t ret = 0;

    p_hfp_ag = os_mem_zalloc2(sizeof(T_HFP_AG));
    if (p_hfp_ag == NULL)
    {
        ret = 1;
        goto fail_alloc_hfp;
    }

    p_hfp_ag->link_num = link_num;
    p_hfp_ag->link_list = os_mem_zalloc2(p_hfp_ag->link_num * sizeof(T_HFP_AG_LINK));
    if (p_hfp_ag->link_list == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    p_hfp_ag->brsf_cpbs = hfp_ag_brsf_cpbs;
    p_hfp_ag->supt_voice_codec = voice_codec;
    p_hfp_ag->ag_cback = cback;

    if (rfc_reg_cb(rfc_hfp_chann_num, hfp_ag_handle_rfc_msg, &p_hfp_ag->hfp_rfc_index) == false)
    {
        ret = 3;
        goto fail_reg_hfp_ag_rfc_cb;
    }

    if (rfc_reg_cb(rfc_hsp_chann_num, hfp_ag_handle_rfc_msg, &p_hfp_ag->hsp_rfc_index) == false)
    {
        ret = 4;
        goto fail_reg_hsp_ag_rfc_cb;
    }

    return true;

fail_reg_hsp_ag_rfc_cb:
fail_reg_hfp_ag_rfc_cb:
    os_mem_free(p_hfp_ag->link_list);
fail_alloc_link:
    os_mem_free(p_hfp_ag);
    p_hfp_ag = NULL;
fail_alloc_hfp:
    PROFILE_PRINT_ERROR1("hfp_ag_init: failed %d", -ret);
    return false;
}

bool hfp_ag_remote_capabilities_get(uint8_t   bd_addr[6],
                                    uint16_t *cpbs)
{
    T_HFP_AG_LINK *p_link;

    p_link = hfp_ag_find_link_by_addr(bd_addr);
    if (p_link != NULL)
    {
        *cpbs = p_link->cpbs;
        return true;
    }

    return false;
}

bool hfp_ag_local_capabilities_get(uint16_t *cpbs)
{
    if (p_hfp_ag != NULL)
    {
        *cpbs = p_hfp_ag->brsf_cpbs;
        return true;
    }

    return false;
}
#endif
