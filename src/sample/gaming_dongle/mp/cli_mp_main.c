/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cli.h"
#include "test_mode.h"



static bool mp_help(const char *cmd_str, char *p_buf, size_t buf_len);

/*
 * The callback function that is executed when "exit" is entered.
 * This is the default subcommand that is always present.
 */
static bool mp_exit(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Exit from mp mode...\r\n");

    return false;
}

/*
 * The callback function that is executed when "mp" is entered.
 */
static bool mp_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Enter mp mode...\r\n");

    return false;
}

/*
 * The callback function that is executed when "hci_mode" is entered.
 */
static bool mp_hci(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Enter hci mode...\r\n");

    switch_into_hci_mode();
    return false;
}

/*
 * The definition of the "hci_mode" subcommand of mp.
 */
const static T_CLI_CMD mp_cmd_hci  =
{
    NULL,           /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "hci_mode",     /* The command string to type. */
    "\r\nhci_mode:\r\n Enter hci mode\r\n",
    mp_hci,         /* The function to run. */
    0               /* One parameter is expected. */
};

/*
 * The definition of the "exit" subcommand of mp.
 * This command is always registered next to "help" command.
 */
const static T_CLI_CMD mp_cmd_exit  =
{
    &mp_cmd_hci,    /* Next command pointed to "reset". */
    NULL,           /* Next subcommand. */
    "exit",         /* The command string to type. */
    "\r\nexit:\r\n Exits from mp mode\r\n",
    mp_exit,        /* The function to run. */
    0               /* Zero parameter is expected. */
};

/*
 * The definition of the "help" subcommand of mp.
 * This command is always at the front of the list of registered commands.
 */
const static T_CLI_CMD mp_cmd_help  =
{
    &mp_cmd_exit,   /* Next command pointed to "exit" command. */
    NULL,           /* Next subcommand. */
    "help",         /* The command string to type. */
    "\r\nhelp:\r\n Lists all the registered commands\r\n",
    mp_help,        /* The function to run. */
    -1              /* Zero or One parameter is expected. */
};

/*
 * The definition of the "mp" command.
 * This command should be registered into file system command list.
 */
static T_CLI_CMD mp_cmd_entry  =
{
    NULL,          /* Next command will be determined when registered into file system. */
    &mp_cmd_help,  /* Next subcommand pointer to "help" subcommand. */
    "mp",          /* The command string to type. */
    "\r\nmp:\r\n Enters mp mode\r\n",
    mp_mode,       /* The function to run. */
    0              /* Zero or One parameter is expected. */
};

/**
 * The definition of the list of commands.
 * Registered commands are added to this list.
 */
const static T_CLI_CMD *p_mp_cmd_list = &mp_cmd_help;

/*
 * The callback function that is executed when "help" is entered.
 * This is the default subcommand that is always present.
 */
static bool mp_help(const char *cmd_str, char *p_buf, size_t buf_len)
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
            p_bud_cmd = &mp_cmd_help;
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
        for (p_bud_cmd = p_mp_cmd_list; p_bud_cmd != NULL; p_bud_cmd = p_bud_cmd->p_next)
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

bool mp_cmd_str_register(void)
{
    return cli_cmd_register(&mp_cmd_entry);
}
