/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "os_mem.h"
#include "os_msg.h"
#include "bt_types.h"
#include "cli.h"
#include "bqb.h"

#include "app_msg.h"
#include "app_main.h"

bool bqb_send_msg(T_IO_CONSOLE subtype, void *param_buf)
{
    uint8_t  event;
    T_IO_MSG msg;

    event = EVENT_IO_TO_APP;

    msg.type    = IO_MSG_TYPE_CONSOLE;
    msg.subtype = subtype;
    msg.u.buf   = param_buf;

    if (os_msg_send(audio_io_queue_handle, &msg, 0) == true)
    {
        return os_msg_send(audio_evt_queue_handle, &event, 0);
    }

    return false;
}

bool bqb_reset(const char *cmd_str, char *buf, size_t buf_len)
{
    void *param_buf;

    (void)cmd_str;

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        LE_UINT16_TO_ARRAY(param_buf, BQB_CMD_RESET);
        bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf);
    }

    snprintf(buf, buf_len, "Reset from BQB mode.\r\n");

    return false;
}

bool bqb_power(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    uint8_t     action;
    void       *param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p_param[param_len] = '\0';

    if (!strcmp(p_param, "on"))
    {
        action = BQB_ACTION_POWER_ON;
    }
    else if (!strcmp(p_param, "off"))
    {
        action = BQB_ACTION_POWER_OFF;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, BQB_CMD_POWER);
        LE_UINT8_TO_STREAM(p, action);

        bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf);
    }

    snprintf(buf, buf_len, "Power %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (on/off).\r\n", p_param);
    return false;
}

bool bqb_pair(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    uint8_t     action;
    void       *param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p_param[param_len] = '\0';

    if (!strcmp(p_param, "start"))
    {
        action = BQB_ACTION_PAIR_START;
    }
    else if (!strcmp(p_param, "stop"))
    {
        action = BQB_ACTION_PAIR_STOP;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, BQB_CMD_PAIR);
        LE_UINT8_TO_STREAM(p, action);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }

    snprintf(buf, buf_len, "Pair %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (start/stop).\r\n", p_param);
    return false;
}

bool bqb_sdp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    uint8_t     action;
    uint16_t    uuid16;
    uint8_t     addr[6];
    uint8_t     i;
    void       *param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p_param[param_len] = '\0';

    action = BQB_ACTION_SDP_SEARCH;

    if (!strcmp(p_param, "uuid_l2cap"))
    {
        uuid16 = UUID_L2CAP;
    }
    else if (!strcmp(p_param, "uuid_avdtp"))
    {
        uuid16 = UUID_AVDTP;
    }
    else if (!strcmp(p_param, "uuid_a2dp"))
    {
        uuid16 = UUID_ADVANCED_AUDIO_DISTRIBUTION;
    }
    else if (!strcmp(p_param, "uuid_audio_source"))
    {
        uuid16 = UUID_AUDIO_SOURCE;
    }
    else
    {
        goto err;
    }

    p_param += param_len + 1;

    for (i = 0; i < 6; i++)
    {
        addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 20);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, BQB_CMD_SDP);
        LE_UINT8_TO_STREAM(p, action);
        LE_UINT16_TO_STREAM(p, uuid16);
        ARRAY_TO_STREAM(p, addr, 6);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }

    snprintf(buf, buf_len, "SDP search %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (uuid).\r\n", p_param);
    return false;
}

bool bqb_avdtp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "open"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVDTP_OPEN;
    }
    else if (!strcmp(subcmd, "start"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVDTP_START;
    }
    else if (!strcmp(subcmd, "close"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVDTP_CLOSE;
    }
    else if (!strcmp(subcmd, "abort"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVDTP_ABORT;
    }
    else if (!strcmp(subcmd, "connect"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;

        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "signal"))
        {
            action = BQB_ACTION_AVDTP_CONNECT_SIGNAL;
        }
        else if (!strcmp(subcmd, "stream"))
        {
            action = BQB_ACTION_AVDTP_CONNECT_STREAM;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVDTP_DISCONNECT;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, BQB_CMD_AVDTP);
        LE_UINT8_TO_STREAM(p, action);
        ARRAY_TO_STREAM(p, addr, 6);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }

    snprintf(buf, buf_len, "AVDTP %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (discover/connect signal|stream).\r\n", subcmd);
    return false;
}

bool bqb_avrcp(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     vol;
    uint8_t     addr[6];
    uint8_t     i;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "connect"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_CONNECT;
    }
    else if (!strcmp(subcmd, "connect_controller"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_CONNECT_CONTROLLER;
    }
    else if (!strcmp(subcmd, "connect_target"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_CONNECT_TARGET;
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_DISCONNECT;
    }
    else if (!strcmp(subcmd, "get_play_status"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_GET_PLAY_STATUS;
    }
    else if (!strcmp(subcmd, "get_element_attr"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_GET_ELEMENT_ATTR;
    }
    else if (!strcmp(subcmd, "play"))
    {
        if (param_num != 7)
        {
            goto err;
        }
        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_PLAY;
    }
    else if (!strcmp(subcmd, "pause"))
    {
        if (param_num != 7)
        {
            goto err;
        }
        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_PAUSE;
    }
    else if (!strcmp(subcmd, "stop"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_STOP;
    }
    else if (!strcmp(subcmd, "rewind"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_REWIND;
    }
    else if (!strcmp(subcmd, "firstforward"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_FASTFORWARD;
    }
    else if (!strcmp(subcmd, "forward"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_FORWARD;
    }
    else if (!strcmp(subcmd, "backward"))
    {
        if (param_num != 7)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_BACKWARD;
    }
    else if (!strcmp(subcmd, "notify_volume"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        vol = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);
        p_param = p_param + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_AVRCP_NOTIFY_VOLUME;
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, BQB_CMD_AVRCP);
        LE_UINT8_TO_STREAM(p, action);

        if (action == BQB_ACTION_AVRCP_NOTIFY_VOLUME)
        {
            LE_UINT8_TO_STREAM(p, vol);
        }

        ARRAY_TO_STREAM(p, addr, 6);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }

    snprintf(buf, buf_len, "AVRCP %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (connect/get_play_status/backward).\r\n", subcmd);
    return false;
}

bool bqb_rfcomm(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "connect"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "spp"))
        {
            action = BQB_ACTION_RFCOMM_CONNECT_SPP;
        }
        else if (!strcmp(subcmd, "hfp"))
        {
            action = BQB_ACTION_RFCOMM_CONNECT_HFP;
        }
        else if (!strcmp(subcmd, "hsp"))
        {
            action = BQB_ACTION_RFCOMM_CONNECT_HSP;
        }
        else if (!strcmp(subcmd, "pbap"))
        {
            action = BQB_ACTION_RFCOMM_CONNECT_PBAP;
        }
        else
        {
            goto err;
        }
        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "spp"))
        {
            action = BQB_ACTION_RFCOMM_DISCONNECT_SPP;
        }
        else if (!strcmp(subcmd, "hfp"))
        {
            action = BQB_ACTION_RFCOMM_DISCONNECT_HFP;
        }
        else if (!strcmp(subcmd, "hsp"))
        {
            action = BQB_ACTION_RFCOMM_DISCONNECT_HSP;
        }
        else if (!strcmp(subcmd, "pbap"))
        {
            action = BQB_ACTION_RFCOMM_DISCONNECT_PBAP;
        }
        else if (!strcmp(subcmd, "all"))
        {
            action = BQB_ACTION_RFCOMM_DISCONNECT_ALL;
        }
        else
        {
            goto err;
        }
        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, BQB_CMD_RFCOMM);
        LE_UINT8_TO_STREAM(p, action);
        ARRAY_TO_STREAM(p, addr, 6);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }
    snprintf(buf, buf_len, "RFCOMM %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (RFCOMM connect/disconnect spp).\r\n", subcmd);
    return false;
}

bool bqb_hfhs(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    uint8_t     level;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "connect"))
    {
        if (param_num != 8)
        {
            goto err;
        }
        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "sco"))
        {
            action = BQB_ACTION_HFHS_CONNECT_SCO;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "disconnect"))
    {
        if (param_num != 8)
        {
            goto err;
        }
        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "sco"))
        {
            action = BQB_ACTION_HFHS_DISCONNECT_SCO;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "call"))
    {
        if (param_num != 8)
        {
            goto err;
        }
        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "active"))
        {
            action = BQB_ACTION_HFHS_CALL_ACTIVE;
        }
        else if (!strcmp(subcmd, "end"))
        {
            action = BQB_ACTION_HFHS_CALL_END;
        }
        else if (!strcmp(subcmd, "reject"))
        {
            action = BQB_ACTION_HFHS_CALL_REJECT;
        }
        else if (!strcmp(subcmd, "answer"))
        {
            action = BQB_ACTION_HFHS_CALL_ANSWER;
        }
        else if (!strcmp(subcmd, "redial"))
        {
            action = BQB_ACTION_HFHS_CALL_REDIAL;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "voice_recognition"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "activate"))
        {
            action = BQB_ACTION_HFHS_VOICE_RECOGNITION_ACTIVATE;
        }
        else if (!strcmp(subcmd, "deactivate"))
        {
            action = BQB_ACTION_HFHS_VOICE_RECOGNITION_DEACTIVATE;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "spk_gain_level_report"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;
        level = (uint8_t)strtol(cli_param_get(p_param, 0, &param_len), NULL, 0);
        p_param = p_param + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }

        action = BQB_ACTION_HFHS_SPK_GAIN_LEVEL_REPORT;
    }
    else if (!strcmp(subcmd, "spk"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "up"))
        {
            action = BQB_ACTION_HFHS_SPK_UP;
        }
        else if (!strcmp(subcmd, "down"))
        {
            action = BQB_ACTION_HFHS_SPK_DOWN;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else if (!strcmp(subcmd, "mic"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "up"))
        {
            action = BQB_ACTION_HFHS_MIC_UP;
        }
        else if (!strcmp(subcmd, "down"))
        {
            action = BQB_ACTION_HFHS_MIC_DOWN;
        }
        else
        {
            goto err;
        }

        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, BQB_CMD_HFHS);
        LE_UINT8_TO_STREAM(p, action);

        if (action == BQB_ACTION_HFHS_SPK_GAIN_LEVEL_REPORT)
        {
            LE_UINT8_TO_STREAM(p, level);
        }

        ARRAY_TO_STREAM(p, addr, 6);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }

    snprintf(buf, buf_len, "HFHS %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (hfhs connect/disconnect).\r\n", subcmd);
    return false;
}

bool bqb_pbap(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *subcmd;
    char       *p_param;
    int32_t     param_num;
    uint32_t    param_len;
    uint8_t     action;
    uint8_t     addr[6];
    uint8_t     i;
    void       *param_buf;

    param_num = cli_param_num_get(cmd_str);
    if (param_num < 1)
    {
        subcmd = (char *)cmd_str;
        goto err;
    }

    subcmd = (char *)cli_param_get(cmd_str, 1, &param_len);
    subcmd[param_len] = '\0';

    if (!strcmp(subcmd, "vcard"))
    {
        if (param_num != 8)
        {
            goto err;
        }

        subcmd += param_len + 1;
        subcmd = (char *)cli_param_get(subcmd, 0, &param_len);
        subcmd[param_len] = '\0';

        if (!strcmp(subcmd, "srm"))
        {
            action = BQB_ACTION_PBAP_VCARD_SRM;
        }
        else if (!strcmp(subcmd, "nosrm"))
        {
            action = BQB_ACTION_PBAP_VCARD_NOSRM;
        }
        else if (!strcmp(subcmd, "entry"))
        {
            action = BQB_ACTION_PBAP_VCARD_ENTRY;
        }
        else
        {
            goto err;
        }
        p_param = subcmd + param_len + 1;

        for (i = 0; i < 6; i++)
        {
            addr[i] = (uint8_t)strtol(cli_param_get(p_param, i, &param_len), NULL, 0);
        }
    }
    else
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 40);
    if (param_buf != NULL)
    {
        uint8_t    *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, BQB_CMD_PBAP);
        LE_UINT8_TO_STREAM(p, action);
        ARRAY_TO_STREAM(p, addr, 6);

        if (bqb_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
            goto err;
        }
    }
    snprintf(buf, buf_len, "PBAP %s from BQB mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (PBAP connect/disconnect rfcomm).\r\n", subcmd);
    return false;
}
