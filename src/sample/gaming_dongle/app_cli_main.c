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
#include "app_msg.h"
#include "app_main.h"
#include "cli.h"
#include "app_cli_main.h"
#include "app_cfg.h"
#include "app_ctrl_cfg.h"
#include "trace.h"

static bool legacy_cli_help(const char *cmd_str, char *p_buf, size_t buf_len);

static bool app_cli_help(const char *cmd_str, char *p_buf, size_t buf_len);

bool app_cli_send_msg(T_IO_CONSOLE subtype, void *param_buf)
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

/*
 * The callback function that is executed when "exit" is entered.
 * This is the default subcommand that is always present.
 */
static bool app_cli_exit(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Exit from app cli mode...\r\n");

    return false;
}

/*
 * The callback function that is executed when "mp" is entered.
 */
static bool app_cli_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Enter app cli mode...\r\n");

    return false;
}

/*
 * The callback function that is executed when "hci_mode" is entered.
 */
static bool app_cmd(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    uint8_t     action;
    void       *param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p_param[param_len] = '\0';

    if (!strcmp(p_param, "scan"))
    {
        action = APP_ACTION_SCAN;
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

        LE_UINT16_TO_STREAM(p, APP_CMD);
        LE_UINT8_TO_STREAM(p, action);

        if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
        {
            snprintf(buf, buf_len, "app_cmd: send msg err");
            os_mem_free(param_buf);
        }
    }

    snprintf(buf, buf_len, "app cmd %s. err\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s .\r\n", p_param);
    return false;

}

static bool app_cli_bredr_conn(const char *cmd_str, char *buf, size_t buf_len)
{
    char       *p_param;
    uint32_t    param_len;
    void       *param_buf;
    uint8_t     link_no = 0;
    uint8_t     action = 0;
    uint8_t *p;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    if (!p_param)
    {
        goto err;
    }
    link_no = strtoul(p_param, NULL, 10);
    p_param += param_len + 1;

    p_param = (char *)cli_param_get(p_param, 0, &param_len);
    action = strtoul(p_param, NULL, 10);

    /* It must be link 0 or link 1 */
    if (link_no != 0 && link_no != 1)
    {
        goto err;
    }

    /* The number one means key short press while the two means key long press.
     * */
    if (action != 1 && action != 2)
    {
        goto err;
    }

    param_buf = os_mem_alloc(RAM_TYPE_DATA_ON, 4);
    if (!param_buf)
    {
        snprintf(buf, buf_len, "bredr_conn: alloc failed\r\n");
        return false;
    }

    p = param_buf;

    LE_UINT16_TO_STREAM(p, 0x5dc0);
    LE_UINT8_TO_STREAM(p, link_no);
    LE_UINT8_TO_STREAM(p, action);

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
    {
        os_mem_free(param_buf);
    }

    snprintf(buf, buf_len, "bredrconn: %s\r\n", cmd_str);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s .\r\n", p_param);
    return false;

}

static bool app_get_media_device(const char *cmd_str, char *buf, size_t buf_len)
{
    char p_param[10] = {'\0'};

    if (app_cfg_const.dongle_media_device == 0)
    {
        strcpy(p_param, "usb");
    }
    else if (app_cfg_const.dongle_media_device == 1)
    {
        strcpy(p_param, "line-in");
    }
    else if (app_cfg_const.dongle_media_device == 2)
    {
        strcpy(p_param, "mic");
    }
    else if (app_cfg_const.dongle_media_device == 3)
    {
        strcpy(p_param, "spdif");
    }

    snprintf(buf, buf_len, "media device %s.\r\n", p_param);


    return false;
}

static int isxdigit(char c)
{
    switch (c)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return 1;
    default:
        return 0;
    }
}

static int ba_check(char *str)
{
    if (!str)
    {
        return -1;
    }
    if (strlen(str) != 17)
    {
        return -1;
    }
    while (*str)
    {
        if (!isxdigit(*str++))
        {
            return -1;
        }
        if (!isxdigit(*str++))
        {
            return -1;
        }
        if (*str == 0)
        {
            break;
        }
        if (*str++ != ':')
        {
            return -1;
        }
    }
    return 0;
}

static int str_to_ba(char *str, uint8_t ba[6])
{
    int i;

    if (ba_check(str) < 0)
    {
        memset(ba, 0, 6);
        return -1;
    }

    for (i = 5; i >= 0; i--, str += 3)
    {
        ba[i] = strtoul(str, NULL, 16);
    }

    return 0;
}

static bool gaming_cli_removebond(const char *cmd_str, char *buf, size_t buf_len)
{
    char *s = NULL;
    char *p = NULL;
    char *tbuf = NULL;
    uint32_t len;
    uint8_t tlen;
    uint8_t sel = 0;
    uint8_t id;
    uint8_t ba[6];
    char ba_str[18];

    s = (char *)cli_param_get(cmd_str, 1, &len);
    if (!s)
    {
        goto err;
    }

    sel = strtoul(s, NULL, 10);
    s += len + 1;
    s = (char *)cli_param_get(s, 0, &len);
    if (!s)
    {
        goto err;
    }

    /* FIXME: opcode + sel + ..., how much space should be allocated? */
    tlen = 3 + 13;

    tbuf = os_mem_alloc(RAM_TYPE_DATA_ON, tlen);
    if (!tbuf)
    {
        snprintf(buf, buf_len, "removebond: alloc buf err\r\n");
        return false;
    }
    p = tbuf;

    switch (sel)
    {
    case 0:
        id = strtoul(s, NULL, 10);
        LE_UINT16_TO_STREAM(p, GAMING_RM_BY_ID_OPCODE);
        LE_UINT8_TO_STREAM(p, id);
        break;
    case 1:
        if (len >= 17)
        {
            memcpy(ba_str, s, 17);
            ba_str[17] = 0;
        }
        else
        {
            goto err;
        }
        str_to_ba(ba_str, ba);
        LE_UINT16_TO_STREAM(p, GAMING_RM_BY_ADDR_OPCODE);
        memcpy(p, ba, 6);
        break;
    default:
        goto err;
    }

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, tbuf) == false)
    {
        snprintf(buf, buf_len, "removebond: send msg err");
        os_mem_free(tbuf);
    }

    snprintf(buf, buf_len, "removebond: %s\r\n", cmd_str);
    return false;

err:
    snprintf(buf, buf_len, "removebond: invalid param %s \r\n", cmd_str);
    if (tbuf)
    {
        os_mem_free(tbuf);
    }
    return false;
}

static bool gaming_cli_disconnect(const char *cmd_str, char *buf, size_t buf_len)
{
    char *s = NULL;
    char *p = NULL;
    char *tbuf = NULL;
    uint32_t len;
    uint8_t tlen;
    uint8_t sel = 0;
    uint8_t id;
    uint8_t ba[6];
    char ba_str[18];

    s = (char *)cli_param_get(cmd_str, 1, &len);
    if (!s)
    {
        goto err;
    }

    sel = strtoul(s, NULL, 10);
    s += len + 1;
    s = (char *)cli_param_get(s, 0, &len);
    if (!s)
    {
        goto err;
    }

    /* FIXME: opcode + sel + ..., how much space should be allocated? */
    tlen = 3 + 13;

    tbuf = os_mem_alloc(RAM_TYPE_DATA_ON, tlen);
    if (!tbuf)
    {
        snprintf(buf, buf_len, "disconnect: alloc buf err\r\n");
        return false;
    }
    p = tbuf;

    switch (sel)
    {
    case 0:
        id = strtoul(s, NULL, 10);
        LE_UINT16_TO_STREAM(p, GAMING_DISC_BY_ID_OPCODE);
        LE_UINT8_TO_STREAM(p, id);
        break;
    case 1:
        if (len >= 17)
        {
            memcpy(ba_str, s, 17);
            ba_str[17] = 0;
        }
        else
        {
            goto err;
        }
        str_to_ba(ba_str, ba);
        LE_UINT16_TO_STREAM(p, GAMING_DISC_BY_ADDR_OPCODE);
        memcpy(p, ba, 6);
        break;
    default:
        goto err;
    }

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, tbuf) == false)
    {
        snprintf(buf, buf_len, "disconnect: send msg err");
        os_mem_free(tbuf);
    }

    snprintf(buf, buf_len, "disconnect: %s\r\n", cmd_str);
    return false;

err:
    snprintf(buf, buf_len, "disconnect: invalid param %s \r\n", cmd_str);
    if (tbuf)
    {
        os_mem_free(tbuf);
    }
    return false;
}

static bool gaming_cli_connect(const char *cmd_str, char *buf, size_t buf_len)
{
    char *s = NULL;
    char *p = NULL;
    char *tbuf = NULL;
    uint32_t len;
    uint8_t tlen;
    uint8_t id = 0;
    uint8_t ba[6];
    char ba_str[18];

    s = (char *)cli_param_get(cmd_str, 1, &len);
    if (!s)
    {
        goto err;
    }
    id = strtoul(s, NULL, 10);

    s += len + 1;
    s = (char *)cli_param_get(s, 0, &len);
    if (!s)
    {
        goto err;
    }
    if (len >= 17)
    {
        memcpy(ba_str, s, 17);
        ba_str[17] = 0;
    }
    else
    {
        goto err;
    }
    str_to_ba(ba_str, ba);

    tlen = 16;

    tbuf = os_mem_alloc(RAM_TYPE_DATA_ON, tlen);
    if (!tbuf)
    {
        snprintf(buf, buf_len, "connect: alloc buf err\r\n");
        return false;
    }
    p = tbuf;

    LE_UINT16_TO_STREAM(p, GAMING_CONN_OPCODE);
    LE_UINT8_TO_STREAM(p, id);
    memcpy(p, ba, 6);

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, tbuf) == false)
    {
        snprintf(buf, buf_len, "connect: send msg err");
        os_mem_free(tbuf);
    }

    snprintf(buf, buf_len, "connect: %s\r\n", cmd_str);
    return false;

err:
    snprintf(buf, buf_len, "connect: invalid param %s \r\n", cmd_str);
    return false;
}

static bool gaming_cli_discov(const char *cmd_str, char *buf, size_t buf_len)
{
    char *s = NULL;
    char *p = NULL;
    char *tbuf = NULL;
    uint32_t len;
    uint8_t tlen;
    uint8_t enable = 0;

    s = (char *)cli_param_get(cmd_str, 1, &len);
    if (!s)
    {
        goto err;
    }
    enable = strtoul(s, NULL, 10);

    tlen = 8;

    tbuf = os_mem_alloc(RAM_TYPE_DATA_ON, tlen);
    if (!tbuf)
    {
        snprintf(buf, buf_len, "discov: alloc buf err\r\n");
        return false;
    }
    p = tbuf;

    LE_UINT16_TO_STREAM(p, GAMING_DISCOV_OPCODE);
    LE_UINT8_TO_STREAM(p, enable);

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, tbuf) == false)
    {
        snprintf(buf, buf_len, "discov: send msg err");
        os_mem_free(tbuf);
    }

    snprintf(buf, buf_len, "discov: %s\r\n", cmd_str);
    return false;

err:
    snprintf(buf, buf_len, "discov: invalid param %s \r\n", cmd_str);
    return false;
}

static bool gaming_cli_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    return false;
}

static bool gaming_cli_dsp(const char *cmd_str, char *buf, size_t buf_len)
{
#if UAL_CONSOLE_PRINT
    extern void audio_pipe_debug(void);

    audio_pipe_debug();
#endif
    snprintf(buf, buf_len, "dsp: %s\r\n", cmd_str);

    return false;
}

static bool legacy_set_bt_conn_state_handle(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;
    char       *p_param;
    uint32_t    param_len;

    param_buf = malloc(3);
    if (!param_buf)
    {
        snprintf(buf, buf_len, "fail: alloc buf err \r\n");
        return false;
    }

    uint8_t *p = param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p[2] = (uint8_t)strtol(p_param, NULL,
                           0);   //SetConnectState 0-Disconnected request, 1-Connect request

    LE_UINT16_TO_STREAM(p, LEGACY_SET_BT_CONN_STATE_OPCODE);

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
    {
        free(param_buf);
        goto err;
    }

    snprintf(buf, buf_len, "send success \r\n");
    return false;
err:
    snprintf(buf, buf_len, "send fail \r\n");
    return false;
}

static bool legacy_pair_to_device_by_addr_handle(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;
    char       *p_param;
    uint32_t    param_len;

    param_buf = malloc(9);
    if (!param_buf)
    {
        snprintf(buf, buf_len, "fail: alloc buf err \r\n");
        return false;
    }

    uint8_t *p = param_buf;

    p_param = (char *)cli_param_get(cmd_str, 1, &param_len);
    p[2] = (uint8_t)strtol(p_param, NULL, 0);   // 0-public, 1-random
    p_param += param_len + 1;

    p_param = (char *)cli_param_get(p_param, 0, &param_len);
    uint8_t i = 0;
    while (i < 6)
    {
        p[8 - i] = (uint8_t)strtol((char *)p_param, (char **)&p_param, 16);
        p_param++;
        i++;
    }
    LE_UINT16_TO_STREAM(p, LEGACY_PAIR_TO_DEVICE_OPCODE);

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
    {
        free(param_buf);
        goto err;
    }

    snprintf(buf, buf_len, "send success \r\n");
    return false;
err:
    snprintf(buf, buf_len, "send fail \r\n");
    return false;
}

static bool legacy_remove_device_handle(const char *cmd_str, char *buf, size_t buf_len)
{
    void       *param_buf;

    param_buf = malloc(2);
    if (!param_buf)
    {
        snprintf(buf, buf_len, "fail: alloc buf err \r\n");
        return false;
    }

    uint8_t *p;

    p = param_buf;

    LE_UINT16_TO_STREAM(p, LEGACY_REMOVE_DEVICE_OPCODE);

    if (app_cli_send_msg(IO_MSG_CONSOLE_STRING_RX, param_buf) == false)
    {
        free(param_buf);
        goto err;
    }

    snprintf(buf, buf_len, "send success \r\n");
    return false;
err:
    snprintf(buf, buf_len, "send fail \r\n");
    return false;
}

static bool legacy_cli_exit(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Exit from legacy cli mode...\r\n");

    return false;
}

static bool legacy_cli_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Enter legacy cli mode...\r\n");

    return false;
}

static T_CLI_CMD legacy_set_bt_conn_state =
{
    NULL,          /* Next command will be determined when registered into file system. */
    NULL,          /* Next subcommand pointer to "help" subcommand. */
    "Legacy_Set_Bt_Conn_State",          /* The command string to type. */
    "\r\nLegacy_Set_Bt_Conn_State:\r\n (state: 0:disconn, 1:conn) ex: Legacy_Set_Bt_Conn_State 00\r\n",
    legacy_set_bt_conn_state_handle,       /* The function to run. */
    1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD legacy_pair_to_device_by_addr =
{
    &legacy_set_bt_conn_state,          /* Next command will be determined when registered into file system. */
    NULL,          /* Next subcommand pointer to "help" subcommand. */
    "Legacy_PairToDev_By_Address",          /* The command string to type. */
    "\r\nLegacy_PairToDev_By_Address:\r\n type bt_addr ex: Legacy_PairToDev_By_Address 01 12-c3-79-99-aA-f1\r\n",
    legacy_pair_to_device_by_addr_handle,       /* The function to run. */
    2              /* Two parameter is expected. */
};

static T_CLI_CMD legacy_remove_dev =
{
    &legacy_pair_to_device_by_addr,          /* Next command will be determined when registered into file system. */
    NULL,          /* Next subcommand pointer to "help" subcommand. */
    "Legacy_Remove_Device",          /* The command string to type. */
    "\r\nLegacy_Remove_Device:\r\n legacy mode remove decive ex: Legacy_Remove_Device\r\n",
    legacy_remove_device_handle,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

const static T_CLI_CMD legacy_cmd_exit  =
{
    &legacy_remove_dev,    /* Next command pointed to "reset". */
    NULL,           /* Next subcommand. */
    "exit",         /* The command string to type. */
    "\r\nexit:\r\n Exits from legacy mode\r\n",
    legacy_cli_exit,        /* The function to run. */
    0               /* Zero parameter is expected. */
};

const static T_CLI_CMD legacy_cmd_help  =
{
    &legacy_cmd_exit,   /* Next command pointed to "exit" command. */
    NULL,           /* Next subcommand. */
    "help",         /* The command string to type. */
    "\r\nhelp:\r\n Lists all the registered commands\r\n",
    legacy_cli_help,        /* The function to run. */
    -1              /* Zero or One parameter is expected. */
};

static T_CLI_CMD legacy_cmd_entry  =
{
    NULL,          /* Next command will be determined when registered into file system. */
    &legacy_cmd_help,  /* Next subcommand pointer to "help" subcommand. */
    "legacy",          /* The command string to type. */
    "\r\nlegacy:\r\n Enters legacy cli mode\r\n",
    legacy_cli_mode,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

static T_CLI_CMD get_media_device =
{
    NULL,          /* Next command will be determined when registered into file system. */
    NULL,          /* Next subcommand pointer to "help" subcommand. */
    "media_device",          /* The command string to type. */
    "\r\nmedia_device:\r\n (0:usb, 1:line-in, 2:mic, 3:spdif)\r\n",
    app_get_media_device,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

static T_CLI_CMD legacy_audio_cmd_entry =
{
    &get_media_device,          /* Next command will be determined when registered into file system. */
    NULL,          /* Next subcommand pointer to "help" subcommand. */
    "bredr_conn",          /* The command string to type. */
    "\r\nbredr_conn:\r\n Connect to a2dp sink device\r\n",
    app_cli_bredr_conn,       /* The function to run. */
    2              /* Zero or One parameter is expected. */
};

/*
 * The definition of the "hci_mode" subcommand of mp.
 */
const static T_CLI_CMD app_cmd_new  =
{
    &legacy_audio_cmd_entry,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "cmd",     /* The command string to type. */
    "\r\ncmd:\r\n process app cmd\r\n",
    app_cmd,         /* The function to run. */
    1               /* One parameter is expected. */
};

/*
 * The definition of the "exit" subcommand of mp.
 * This command is always registered next to "help" command.
 */
const static T_CLI_CMD app_cmd_exit  =
{
    &app_cmd_new,    /* Next command pointed to "reset". */
    NULL,           /* Next subcommand. */
    "exit",         /* The command string to type. */
    "\r\nexit:\r\n Exits from app mode\r\n",
    app_cli_exit,        /* The function to run. */
    0               /* Zero parameter is expected. */
};

/*
 * The definition of the "help" subcommand of mp.
 * This command is always at the front of the list of registered commands.
 */
const static T_CLI_CMD app_cmd_help  =
{
    &app_cmd_exit,   /* Next command pointed to "exit" command. */
    NULL,           /* Next subcommand. */
    "help",         /* The command string to type. */
    "\r\nhelp:\r\n Lists all the registered commands\r\n",
    app_cli_help,        /* The function to run. */
    -1              /* Zero or One parameter is expected. */
};

/*
 * The definition of the "mp" command.
 * This command should be registered into file system command list.
 */
static T_CLI_CMD app_cmd_entry  =
{
    NULL,          /* Next command will be determined when registered into file system. */
    &app_cmd_help,  /* Next subcommand pointer to "help" subcommand. */
    "app",          /* The command string to type. */
    "\r\nmp:\r\n Enters app cli mode\r\n",
    app_cli_mode,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

static T_CLI_CMD gaming_cmd_removebond =
{
    NULL,
    NULL,
    "removebond",
    "\r\nremovebond:\r\nRemove bond info\r\n",
    gaming_cli_removebond,
    2
};

static T_CLI_CMD gaming_cmd_dsp =
{
    &gaming_cmd_removebond,
    NULL,
    "dsp",
    "\r\ndsp:\r\nOutput dsp log\r\n",
    gaming_cli_dsp,
    0
};

static T_CLI_CMD gaming_cmd_disconnect =
{
    &gaming_cmd_dsp,
    NULL,
    "disconnect",
    "\r\ndisconnect:\r\nDisconnect\r\n",
    gaming_cli_disconnect,
    2
};

static T_CLI_CMD gaming_cmd_connect =
{
    &gaming_cmd_disconnect,
    NULL,
    "connect",
    "\r\nconnect: \r\n Connect to remote dev\r\n",
    gaming_cli_connect,
    2 /* idx, bdaddr */
};

static T_CLI_CMD gaming_cmd_discov =
{
    &gaming_cmd_connect,
    NULL,
    "discov",
    "\r\ndiscov:\r\n Start or stop discovery\r\n",
    gaming_cli_discov,
    1
};

static T_CLI_CMD gaming_cmd_entry =
{
    NULL,
    &gaming_cmd_discov,
    "gaming",
    "\r\ngaming:\r\n Enter gaming cli mode\r\n",
    gaming_cli_mode,
    0
};

const static T_CLI_CMD *p_legacy_cmd_list = &legacy_cmd_help;

static bool legacy_cli_help(const char *cmd_str, char *p_buf, size_t buf_len)
{
    static const T_CLI_CMD *p_bud_cmd = NULL;
    const char             *p_param = NULL;
    uint32_t                param_len;
    bool                    ret;

    /* Obtain the specified cmd string that needs to display. */
    p_param = cli_param_get(cmd_str, 1, &param_len);

    /* Display all available commands. */
    if (p_param == NULL)
    {
        if (p_bud_cmd == NULL)
        {
            /* Reset back to the start of the list. */
            p_bud_cmd = &legacy_cmd_help;
        }

        strncpy(p_buf, p_bud_cmd->p_help, buf_len);

        p_bud_cmd = p_bud_cmd->p_next;
        if (p_bud_cmd == NULL)
        {
            /* 'help' command completed. */
            ret = false;
        }
        else
        {
            ret = true;
        }
    }
    else
    {
        for (p_bud_cmd = p_legacy_cmd_list; p_bud_cmd != NULL; p_bud_cmd = p_bud_cmd->p_next)
        {
            if (!strcmp(p_param, p_bud_cmd->p_cmd))
            {
                strncpy(p_buf, p_bud_cmd->p_help, buf_len);
                break;
            }
        }

        if (p_bud_cmd != NULL)
        {
            /* Found the specific cmd. */
            p_bud_cmd = NULL;
        }
        else
        {
            /* The specific cmd was not supported. */
            snprintf(p_buf, buf_len,
                     "Incorrect command '%s'. Enter 'help' to view available commands.\r\n",
                     p_param);
        }
        ret = false;
    }
    return ret;
}

/**
 * The definition of the list of commands.
 * Registered commands are added to this list.
 */
const static T_CLI_CMD *p_app_cmd_list = &app_cmd_help;

/*
 * The callback function that is executed when "help" is entered.
 * This is the default subcommand that is always present.
 */
static bool app_cli_help(const char *cmd_str, char *p_buf, size_t buf_len)
{
    static const T_CLI_CMD *p_bud_cmd = NULL;
    const char             *p_param = NULL;
    uint32_t                param_len;
    bool                    ret;

    /* Obtain the specified cmd string that needs to display. */
    p_param = cli_param_get(cmd_str, 1, &param_len);

    /* Display all available commands. */
    if (p_param == NULL)
    {
        if (p_bud_cmd == NULL)
        {
            /* Reset back to the start of the list. */
            p_bud_cmd = &app_cmd_help;
        }

        strncpy(p_buf, p_bud_cmd->p_help, buf_len);

        p_bud_cmd = p_bud_cmd->p_next;
        if (p_bud_cmd == NULL)
        {
            /* 'help' command completed. */
            ret = false;
        }
        else
        {
            ret = true;
        }
    }
    else
    {
        for (p_bud_cmd = p_app_cmd_list; p_bud_cmd != NULL; p_bud_cmd = p_bud_cmd->p_next)
        {
            if (!strcmp(p_param, p_bud_cmd->p_cmd))
            {
                strncpy(p_buf, p_bud_cmd->p_help, buf_len);
                break;
            }
        }

        if (p_bud_cmd != NULL)
        {
            /* Found the specific cmd. */
            p_bud_cmd = NULL;
        }
        else
        {
            /* The specific cmd was not supported. */
            snprintf(p_buf, buf_len,
                     "Incorrect command '%s'. Enter 'help' to view available commands.\r\n",
                     p_param);
        }

        ret = false;
    }

    return ret;


}

bool app_cmd_register(void)
{
    cli_cmd_register(&app_cmd_entry);
    cli_cmd_register(&gaming_cmd_entry);
    cli_cmd_register(&legacy_cmd_entry);

    return true;
}
