/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if UAL_CONSOLE_PRINT
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "os_mem.h"
#include "os_msg.h"
#include "console.h"
#include "cli.h"
#include "app_msg.h"
#include "app_main.h"

#include "cli_console_cmd.h"
#include "app_usb_layer.h"
#include "bass_client.h"
#include "trace.h"

#define REMOTE_NAME_MAX_LEN     36


bool test_send_msg(T_IO_CONSOLE subtype, void *param_buf)
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

static bool set_white_list(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 10);
    if (param_buf != NULL)
    {
        uint8_t *p = param_buf;
        LE_UINT16_TO_STREAM(p, 0x5dF1);
        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[0] = (uint8_t)strtol(p_param, NULL, 0);   // 0 clear, 1 add, 2 remove
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[1] = (uint8_t)strtol(p_param, NULL, 0);   //bd_type
        p_param += param_len + 1;

        uint8_t i = 0;
        while (i < 6)
        {
            p[2 + i] = (uint8_t)strtol((char *)p_param, (char **)&p_param, 16);
            p_param++;
            i++;
        }
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool set_pair_filter(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, REMOTE_NAME_MAX_LEN + 4);
    if (param_buf != NULL)
    {
        uint8_t *p = param_buf;
        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);   // 0 disable filter, 1 enable filter
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);   //0 filter type bd-addr, 1 filter type name
        p_param += param_len + 1;
        p_param = (char *)cli_param_get(p_param, 0, &param_len);

        if (p[3] == 0) // bdaddr
        {
            uint8_t i = 0;
            p[4] = (uint8_t)strtol((char *)p_param, (char **)&p_param, 0);
            p_param++;
            while (i < 6)
            {
                p[5 + i] = (uint8_t)strtol((char *)p_param, (char **)&p_param, 16);
                p_param++;
                i++;
            }
        }
        else
        {
            uint8_t name_len = strlen(p_param) > REMOTE_NAME_MAX_LEN ? REMOTE_NAME_MAX_LEN : strlen(p_param);
            memcpy(p + 4, p_param, name_len);
        }
        LE_UINT16_TO_STREAM(p, 0x5dF2);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_autopair(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p = param_buf;

        LE_UINT16_TO_STREAM(p, 0x5dF3);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_enable_unicast(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;
        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, 0x5dF4);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_bass_adv(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;
        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, 0x5dF9);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_unicast_snk_adv(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, 0x5dFA);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_lc3_playback(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, 0x5dFD);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_sirk_calc(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, 0x5dFB);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_dsp_log_flush(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;


        LE_UINT16_TO_STREAM(p, 0x5dF5);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_audio_pipe_debug(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;


        LE_UINT16_TO_STREAM(p, 0x5dF8);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_mic_mute_control(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);   //device index
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);   //enable
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_MIC_MUTE_STATE_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_mcp_key_control(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);   //conn_id
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);   //opcode
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, 0x5dF6);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_mcp_key_press(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, 0x5dFE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_ccp_control(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[4] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, 0x5dFF);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}


static bool cmd_set_coordniators_mute(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_COORDINATORS_MUTE_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_set_coordniators_vol(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_COORDINATORS_VOL_VAL_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_set_remote_volume_offset(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 6);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[4] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[5] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_VOLUME_OFFSET_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_set_remote_mute(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_REMOTE_MUTE_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_set_remote_vol(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_REMOTE_VOL_VAL_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_coor_connect_all(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[2] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[4] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, CONNECT_ALL_SET_MEMBERS);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}

static bool cmd_coor_discover_stop(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;


    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 6);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[4] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[5] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, ENABLE_DISCOVER_SET_MEMBERS);
        LE_UINT8_TO_STREAM(p, 0);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}


static bool cmd_coor_discover_start(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 6);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        p[3] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[4] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        p[5] = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, ENABLE_DISCOVER_SET_MEMBERS);
        LE_UINT8_TO_STREAM(p, 1);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}


static bool cmd_basetremotesync(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     idx, bis;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        idx = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        bis = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, BC_AS_SET_REMOTE_SYNC_OPCODE);
        LE_UINT8_TO_STREAM(p, idx);
        LE_UINT8_TO_STREAM(p, bis);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}

static bool cmd_bsk_sync_src(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     idx, enable, bis;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        idx = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        bis = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        enable = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, BC_SNK_SYNC_BC_SRC_OPCODE);
        LE_UINT8_TO_STREAM(p, idx);
        LE_UINT8_TO_STREAM(p, bis);
        LE_UINT8_TO_STREAM(p, enable);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_bskselectsrc(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     idx;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        idx = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, BC_SNK_SELEC_BC_SRC_OPCODE);
        LE_UINT8_TO_STREAM(p, idx);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}

static bool cmd_ba_sync_src(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     idx, enable;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        idx = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        enable = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, BC_AS_SYNC_BC_SRC_OPCODE);
        LE_UINT8_TO_STREAM(p, idx);
        LE_UINT8_TO_STREAM(p, enable);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_baselectsrc(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     idx;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        idx = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, BC_AS_SELEC_BC_SRC_OPCODE);
        LE_UINT8_TO_STREAM(p, idx);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}


static bool cmd_broadcast_scan(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, SCAN_START_OPCODE);
        LE_UINT16_TO_STREAM(p, GATT_UUID_BASS);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;

}

static bool cmd_unicast_scan(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, SCAN_START_OPCODE);
        LE_UINT16_TO_STREAM(p, GATT_UUID_ASCS);
        LE_UINT8_TO_STREAM(p, 0);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_bascan(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     enable;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        enable = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, BROADCAST_ASSIS_DISC_OPCODE);
        LE_UINT8_TO_STREAM(p, enable);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;
}


static bool cmd_ascs_op(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     op, qos;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        op = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        qos = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, TRANSFER_OPERATION_OPCODE);
        LE_UINT8_TO_STREAM(p, op);
        LE_UINT8_TO_STREAM(p, qos);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_ascs_set_configure6(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, 3);
        *p = UC_SET_CONFIGURE6;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_ascs_set_configure5(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, 1);
        *p = UC_SET_CONFIGURE5;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_ascs_set_configure4(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;
    uint8_t     chnl_loc;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        chnl_loc = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, chnl_loc);
        *p = UC_SET_CONFIGURE4;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_ascs_set_configure3(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;
    uint8_t     chnl_loc;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        chnl_loc = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, chnl_loc);
        *p = UC_SET_CONFIGURE3;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_ascs_set_configure2(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;
    uint8_t     chnl_loc;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        chnl_loc = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, chnl_loc);
        *p = UC_SET_CONFIGURE2;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_ascs_set_configure(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;
    uint8_t     chnl_loc;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        chnl_loc = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, chnl_loc);
        *p = UC_SET_CONFIGURE;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}


static bool cmd_startsync(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;
    uint8_t     enable;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        enable = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, ENABLE_BST_SYNC_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, enable);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_bs_rmv_snk(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, RMV_DEV_FROM_BSGRP_OPCODE);
        LE_UINT8_TO_STREAM(p, index);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_bs_add_snk(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, ADD_DEV_TO_BSGRP_OPCODE);
        LE_UINT8_TO_STREAM(p, index);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;

}

static bool cmd_releasebst(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, START_BROADCAST_OPCODE);
        LE_UINT8_TO_STREAM(p, 0x02);
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_stopbst(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;
    char       *p_param;
    uint32_t    param_len;
    uint8_t     op;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        op = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, START_BROADCAST_OPCODE);
        LE_UINT8_TO_STREAM(p, op);
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }


    return false;
}

static bool cmd_startbst(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;
        LE_UINT16_TO_STREAM(p, START_BROADCAST_OPCODE);
        LE_UINT8_TO_STREAM(p, 0x01);
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_set_bs_codec(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    void       *param_buf;
    uint8_t     qos_mode;
    uint8_t     idx;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    idx = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;

    p_param = (char *)cli_param_get(p_param, 0, &param_len);
    qos_mode = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;


    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, SET_BROADCAST_TK_CODEC_OPCODE);
        LE_UINT8_TO_STREAM(p, idx);
        LE_UINT8_TO_STREAM(p, qos_mode);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;

}

static bool cmd_bs_set_configure(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;
    uint8_t     chnl_loc;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        p_param = (char *)cli_param_get(p_param, 0, &param_len);
        chnl_loc = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, SET_DEVICE_CONF_OPCODE);
        LE_UINT8_TO_STREAM(p, index);
        LE_UINT8_TO_STREAM(p, chnl_loc);

        *p = BS_SET_CONFIGURE;
        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;

}

static bool cmd_disconnect_device(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    void       *param_buf;
    uint8_t     mode;
    uint8_t     idx;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    idx = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;

    p_param = (char *)cli_param_get(p_param, 0, &param_len);
    mode = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;


    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, DISCONNECT_DEVICE_OPCODE);
        LE_UINT8_TO_STREAM(p, mode);
        LE_UINT8_TO_STREAM(p, idx);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;

}


static bool cmd_connect_device(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    void       *param_buf;
    uint8_t     mode;
    uint8_t     idx;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    idx = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;

    p_param = (char *)cli_param_get(p_param, 0, &param_len);
    mode = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;


    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, CONNECT_DEVICE_OPCODE);
        LE_UINT8_TO_STREAM(p, mode);
        LE_UINT8_TO_STREAM(p, idx);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }
    return false;

}



static bool cmd_remove_bond(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, REMOVE_BOND_OPCODE);
        LE_UINT8_TO_STREAM(p, index);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;

}

static bool cmd_create_bond(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    void       *param_buf;
    uint32_t    param_len;
    uint8_t     index;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
        index = (uint8_t)strtol(p_param, NULL, 0);
        p_param += param_len + 1;

        LE_UINT16_TO_STREAM(p, CREATE_BOND_OPCODE);
        LE_UINT8_TO_STREAM(p, index);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;

}

static bool cmd_stop_escan(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, SCAN_STOP_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;

}


static bool cmd_escan(const char *cmd_str, char *buf, size_t buf_len)
{

    char       *p_param;
    uint32_t    param_len;
    void       *param_buf;
    uint8_t     uuid_low;
    uint8_t     uuid_high;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    uuid_low = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;

    p_param = (char *)cli_param_get(p_param, 0, &param_len);
    uuid_high = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;


    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, SCAN_START_OPCODE);
        LE_UINT8_TO_STREAM(p, uuid_low);
        LE_UINT8_TO_STREAM(p, uuid_high);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_get_devs_info(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 2);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, GET_DEVICES_INFO_OPCODE);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool cmd_set_audio_path(const char *cmd_str, char *buf, size_t buf_len)
{

    char       *p_param;
    uint32_t    param_len;
    void       *param_buf;
    uint8_t     audio_path;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    audio_path = (uint8_t)strtol(p_param, NULL, 0);
    p_param += param_len + 1;


    param_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 3);
    if (param_buf != NULL)
    {
        uint8_t *p;

        p = param_buf;

        LE_UINT16_TO_STREAM(p, 0x5dF7);
        LE_UINT8_TO_STREAM(p, audio_path);

        if (test_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            os_mem_free(param_buf);
        }
    }

    return false;
}

static bool audio_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    return false;
}

extern size_t xFreeBytesRemaining[RAM_TYPE_NUM];

static bool cmd_heapinfo(const char *cmd_str, char *buf, size_t buf_len)
{
    uint8_t tbuf[96];
    uint16_t tlen;

#if TARGET_RTL8773DO
    tlen = snprintf((char *)tbuf, sizeof(tbuf),
                    "ITCM1 %u, DTCM0 %u, DATA_ON %u, DATA_OFF %u, BUFFER_ON %u, BUFFER_OFF %u, DSPSHARE MEM %u\r\n",
                    os_mem_peek(RAM_TYPE_DATA_ON),
                    os_mem_peek(RAM_TYPE_DATA_ON),
                    os_mem_peek(RAM_TYPE_DATA_ON),
                    os_mem_peek(RAM_TYPE_DATA_OFF),
                    os_mem_peek(RAM_TYPE_BUFFER_ON),
                    os_mem_peek(RAM_TYPE_BUFFER_OFF),
                    os_mem_peek(RAM_TYPE_DATA_ON));
#else
    tlen = snprintf((char *)tbuf, sizeof(tbuf),
                    "ITCM1 %u, DTCM0 %u, DATA_ON %u, DATA_OFF %u, BUFFER_ON %u, BUFFER_OFF %u, DSPSHARE MEM %u\r\n",
                    os_mem_peek(RAM_TYPE_ITCM1),
                    os_mem_peek(RAM_TYPE_DTCM0),
                    os_mem_peek(RAM_TYPE_DATA_ON),
                    os_mem_peek(RAM_TYPE_DATA_OFF),
                    os_mem_peek(RAM_TYPE_BUFFER_ON),
                    os_mem_peek(RAM_TYPE_BUFFER_OFF),
                    (uint32_t)xFreeBytesRemaining[RAM_TYPE_DSPSHARE]);
#endif
    console_write(tbuf, tlen);

    return false;
}

static T_CLI_CMD set_white_list_cmd =
{
    NULL,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_white_list",          /* The command string to type. */
    "\r\nfilter:\r\n 0 clear 1 add 2 remove\r\n",
    set_white_list,       /* The function to run. */
    3              /* Zero or One parameter is expected. */
};

static T_CLI_CMD set_pair_filter_cmd =
{
    &set_white_list_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_pair_filter",          /* The command string to type. */
    "\r\nfilter:\r\n 0 disable 1 enable\r\n 0 addr 1 name",
    set_pair_filter,       /* The function to run. */
    3              /* Zero or One parameter is expected. */
};

static T_CLI_CMD autopair_cmd =
{
    &set_pair_filter_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "autopair",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_autopair,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD enable_unicast_cmd =
{
    &autopair_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "enable_unicast",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_enable_unicast,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD mic_mute_cmd =
{
    &enable_unicast_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "mic_mute",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_mic_mute_control,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD mcp_control_cmd =
{
    &mic_mute_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "mcp_control",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_mcp_key_control,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};

static T_CLI_CMD get_devs_info_cmd =
{
    &mcp_control_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "get_info",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_get_devs_info,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD bass_adv_cmd =
{
    &get_devs_info_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bass_adv",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_bass_adv,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD unicast_snk_adv_cmd =
{
    &bass_adv_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "snk_adv",          /* The command string to type. */
    "\r\naudio:\r\n enable\r\n",
    cmd_unicast_snk_adv,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD lc3_playback_cmd =
{
    &unicast_snk_adv_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "playback",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_lc3_playback,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD sir_cal_cmd =
{
    &lc3_playback_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "sirk_calc",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_sirk_calc,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

static T_CLI_CMD dsp_log_flush_cmd =
{
    &sir_cal_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "dsp_flush",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_dsp_log_flush,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD audio_pipe_cmd =
{
    &dsp_log_flush_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "audio_pipe",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_audio_pipe_debug,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD mcp_key_cmd =
{
    &audio_pipe_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "key",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_mcp_key_press,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD ccp_handle_cmd =
{
    &mcp_key_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ccp_ctl",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ccp_control,       /* The function to run. */
    3              /* Zero or One parameter is expected. */
};


static T_CLI_CMD set_coordinators_mute_cmd =
{
    &ccp_handle_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_coor_mute",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_set_coordniators_mute,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD set_coordinators_vol_cmd =
{
    &set_coordinators_mute_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_coor_vol",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_set_coordniators_vol,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD set_remote_vol_off_cmd =
{
    &set_coordinators_vol_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_vol_offset",          /* The command string to type. */
    "\r\naudio:\r\n dev_idx, idx, volume_offset\r\n",
    cmd_set_remote_volume_offset,       /* The function to run. */
    4              /* Zero or One parameter is expected. */
};


static T_CLI_CMD set_remote_mute_cmd =
{
    &set_remote_vol_off_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_remote_mute",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_set_remote_mute,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};

static T_CLI_CMD set_remote_volume_cmd =
{
    &set_remote_mute_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_remote_vol",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_set_remote_vol,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD coor_connect_all_cmd =
{
    &set_remote_volume_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "coor_connect_all",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_coor_connect_all,       /* The function to run. */
    3              /* Zero or One parameter is expected. */
};


static T_CLI_CMD coor_discover_stop_cmd =
{
    &coor_connect_all_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "coor_disc_stop",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_coor_discover_stop,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD coor_discover_start_cmd =
{
    &coor_discover_stop_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "coor_disc_start",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_coor_discover_start,       /* The function to run. */
    3              /* Zero or One parameter is expected. */
};

static T_CLI_CMD ba_set_remote_sync_cmd =
{
    &coor_discover_start_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ba_set_remote_sync",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_basetremotesync,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};

static T_CLI_CMD bsk_sync_src_cmd =
{
    &ba_set_remote_sync_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bsk_src_sync",          /* The command string to type. */
    "\r\naudio:\r\n idx / bis/ enable\r\n",
    cmd_bsk_sync_src,       /* The function to run. */
    3              /* Zero or One parameter is expected. */
};

static T_CLI_CMD bsk_select_src_cmd =
{
    &bsk_sync_src_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bsk_src_select",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_bskselectsrc,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD ba_sync_src_cmd =
{
    &bsk_select_src_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ba_src_sync",          /* The command string to type. */
    "\r\naudio:\r\n idx: 0xXX, enable: 0 or 1\r\n",
    cmd_ba_sync_src,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD ba_select_src_cmd =
{
    &ba_sync_src_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ba_src_select",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_baselectsrc,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD broadcast_scan_op_cmd =
{
    &ba_select_src_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bst_scan",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_broadcast_scan,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

static T_CLI_CMD unicast_scan_op_cmd =
{
    &broadcast_scan_op_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "unicast_scan",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_unicast_scan,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD bascan_op_cmd =
{
    &unicast_scan_op_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bascan",          /* The command string to type. */
    "\r\naudio:\r\n enable \r\n",
    cmd_bascan,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD ascs_op_cmd =
{
    &bascan_op_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ascs_op",          /* The command string to type. */
    "\r\naudio:\r\n  param: mode(1 2) / qos(1 2 3)\r\n",
    cmd_ascs_op,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};

static T_CLI_CMD uc_rmv_conf_cmd =
{
    &ascs_op_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "rmv_conf",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure6,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD uc_set_conf6_cmd =
{
    &uc_rmv_conf_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "conv_in_out",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure6,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD uc_set_conf5_cmd =
{
    &uc_set_conf6_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "conv_out",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure5,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD uc_set_conf4_cmd =
{
    &uc_set_conf5_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "media_stream",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure4,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD uc_set_conf3_cmd =
{
    &uc_set_conf4_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ascs_set_conf3",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure3,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD uc_set_conf2_cmd =
{
    &uc_set_conf3_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "ascs_set_conf2",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure2,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD uc_set_conf_cmd =
{
    &uc_set_conf2_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "media_stream_48k",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_ascs_set_configure,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD start_sync_cmd =
{
    &uc_set_conf_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "startsync",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_startsync,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD bs_rmv_snk_cmd =
{
    &start_sync_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bs_rmv_snk",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_bs_rmv_snk,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD bs_add_snk_cmd =
{
    &bs_rmv_snk_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bs_add_snk",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_bs_add_snk,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD bs_set_conf_cmd =
{
    &bs_add_snk_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "bs_set_conf",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_bs_set_configure,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};

static T_CLI_CMD release_bst_cmd =
{
    &bs_set_conf_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "releasebst",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_releasebst,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};


static T_CLI_CMD stop_bst_cmd =
{
    &release_bst_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "stopbst",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_stopbst,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD start_bst_cmd =
{
    &stop_bst_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "startbst",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_startbst,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

static T_CLI_CMD set_bs_codec_cmd =
{
    &start_bst_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "set_bs_codec",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_set_bs_codec,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD disconnect_cmd =
{
    &set_bs_codec_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "disconn",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_disconnect_device,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD connect_cmd =
{
    &disconnect_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "conn",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_connect_device,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};


static T_CLI_CMD remove_bond_cmd =
{
    &connect_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "removebond",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_remove_bond,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD create_bond_cmd =
{
    &remove_bond_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "createbond",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_create_bond,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};


static T_CLI_CMD stop_scan_cmd =
{
    &create_bond_cmd,           /* Next command will be determined when registered into file system. */
    NULL,                       /* Next subcommand pointer to "help" subcommand. */
    "stopescan",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_stop_escan,       /* The function to run. */
    0               /* Zero or One parameter is expected. */
};


static T_CLI_CMD escan_cmd_entry  =
{
    &stop_scan_cmd,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "escan",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_escan,       /* The function to run. */
    2               /* Zero or One parameter is expected. */
};

static T_CLI_CMD set_audio_path  =
{
    &escan_cmd_entry,           /* Next command will be determined when registered into file system. */
    NULL,  /* Next subcommand pointer to "help" subcommand. */
    "audio_path",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    cmd_set_audio_path,       /* The function to run. */
    1               /* Zero or One parameter is expected. */
};

static T_CLI_CMD output_heapinfo =
{
    &set_audio_path,
    NULL,
    "heapinfo",
    "\r\nHeapinfo:\r\n",
    cmd_heapinfo,
    0
};

static T_CLI_CMD audio_cmd_entry  =
{
    NULL,           /* Next command will be determined when registered into file system. */
    &output_heapinfo,  /* Next subcommand pointer to "help" subcommand. */
    "leaudio",          /* The command string to type. */
    "\r\naudio:\r\n start audio test cmd\r\n",
    audio_mode,       /* The function to run. */
    0               /* Zero or One parameter is expected. */
};

bool console_cmd_register(void)
{
    return cli_cmd_register(&audio_cmd_entry);
}
#endif

