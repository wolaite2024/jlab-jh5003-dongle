/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cli.h"
#include "cli_cfg_cmd.h"

static bool cfg_help(const char *cmd_str, char *p_buf, size_t buf_len);


/*
 * The callback function that is executed when "exit" is entered.
 * This is the default subcommand that is always present.
 */
static bool cfg_exit(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Exit from cfg mode...\r\n");

    return false;
}

static bool cfg_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;

    /* Ensure a null terminator after each character written. */
    memset(buf, 0x00, buf_len);

    snprintf(buf, buf_len, "Enter cfg mode...\r\n");

    return false;
}

const static T_CLI_CMD cfg_cmd_dump  =
{
    NULL,   /* Next command pointed to NULL. */
    NULL,           /* Next subcommand. */
    "dump",         /* The command string to type. */
    "\r\ndump:\r\n example:dump app/appnv/sys/sysnv\r\n",
    cfg_dump,       /* The function to run. */
    1              /* One parameter is expected. */
};

/*
 * The definition of the "exit" subcommand of bud.
 * This command is always registered next to "help" command.
 */
const static T_CLI_CMD cfg_cmd_exit  =
{
    &cfg_cmd_dump, /* Next command pointed to "reset". */
    NULL,           /* Next subcommand. */
    "exit",         /* The command string to type. */
    "\r\nexit:\r\n Exits from cfg mode\r\n",
    cfg_exit,       /* The function to run. */
    0               /* Zero parameter is expected. */
};

/*
 * The definition of the "help" subcommand of bud.
 * This command is always at the front of the list of registered commands.
 */
const static T_CLI_CMD cfg_cmd_help  =
{
    &cfg_cmd_exit,  /* Next command pointed to "exit" command. */
    NULL,           /* Next subcommand. */
    "help",         /* The command string to type. */
    "\r\nhelp:\r\n Lists all the registered commands\r\n",
    cfg_help,       /* The function to run. */
    -1              /* Zero or One parameter is expected. */
};

/*
 * The definition of the "bud" command.
 * This command should be registered into file system command list.
 */
static T_CLI_CMD cfg_cmd_entry  =
{
    NULL,           /* Next command will be determined when registered into file system. */
    &cfg_cmd_help,  /* Next subcommand pointer to "help" subcommand. */
    "cfg",          /* The command string to type. */
    "\r\ncfg:\r\n Enters cfg mode\r\n",
    cfg_mode,       /* The function to run. */
    0               /* Zero or One parameter is expected. */
};


/**
 * The definition of the list of commands.
 * Registered commands are added to this list.
 */
const static T_CLI_CMD *p_cfg_cmd_list = &cfg_cmd_help;


/*
 * The callback function that is executed when "help" is entered.
 * This is the default subcommand that is always present.
 */
static bool cfg_help(const char *cmd_str, char *p_buf, size_t buf_len)
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
            p_bud_cmd = &cfg_cmd_help;
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
        for (p_bud_cmd = p_cfg_cmd_list; p_bud_cmd != NULL; p_bud_cmd = p_bud_cmd->p_next)
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

bool cfg_cmd_register(void)
{
    return cli_cmd_register(&cfg_cmd_entry);
}


