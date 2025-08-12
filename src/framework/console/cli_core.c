/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "os_sync.h"
#include "os_mem.h"
#include "console.h"
#include "cli_core.h"
#include "cli.h"

/* The command console prompt string. */
static char cli_prompt[CLI_PROMPT_SIZE] = {0};

/**
 * The cached command console prompt string that is used to store command
 * prompt when entering subcommand mode.
 */
static char cli_cached_prompt[CLI_PROMPT_SIZE] = {0};

/**
 * The callback function that is executed when "help" is entered.
 * This is the default command that is always present.
 */
static bool cli_help(const char *cmd_str, char *buf, size_t buf_len);

/**
 * The callback function that is executed when "exit" is entered.
 * This is the default command that is always present.
 */
static bool cli_exit(const char *cmd_str, char *buf, size_t buf_len);

/**
 * The callback function that is executed when "mode" is entered.
 * This is the default command that is always present.
 */
static bool cli_mode(const char *cmd_str, char *buf, size_t buf_len);

/**
 * The definition of the "mode" command.
 * This command is always at the front of the list of registered commands.
 */
static T_CLI_CMD cli_cmd_mode =
{
    NULL,           /* Next command. */
    NULL,           /* Next subcommand. */
    "mode",         /* The command string to type. */
    "\r\nmode:\r\n Changes console mode(string or binary)\r\n\r\n",
    cli_mode,       /* The function to run. */
    1,              /* One parameter is expected. */
    NULL            /* Default command match pattern. */
};

/**
 * The definition of the "exit" command.
 * This command is always registered next to "help" command.
 */
static T_CLI_CMD cli_cmd_exit =
{
    &cli_cmd_mode,  /* Next command pointed to "mode" command. */
    NULL,           /* Next subcommand. */
    "exit",         /* The command string to type. */
    "\r\nexit:\r\n Exits from the terminal\r\n",
    cli_exit,       /* The function to run. */
    0,              /* Zero parameter is expected. */
    NULL            /* Default command match pattern. */
};

/**
 * The definition of the "help" command.
 * This command is always at the front of the list of registered commands.
 */
static T_CLI_CMD cli_cmd_help =
{
    &cli_cmd_exit,  /* Next command pointed to "exit" command. */
    NULL,           /* Next subcommand. */
    "help",         /* The command string to type. */
    "\r\nhelp:\r\n Lists all the registered commands\r\n\r\n",
    cli_help,       /* The function to run. */
    -1,             /* Zero or One parameter is expected. */
    NULL            /* Default command match pattern. */
};

/**
 * The definition of the list of commands.
 * Registered commands are added to this list.
 */
static T_CLI_CMD *p_cli_cmd_list = &cli_cmd_help;
static T_CLI_CMD *p_last_cli_cmd = &cli_cmd_mode;

/**
 * The definition of the list of subcommands.
 */
const static T_CLI_CMD *p_cli_subcmd_list = NULL;

/**
 * The definition of the CLI subcommand mode flag.
 * Set true if CLI enters subcommand mode.
 * Set false if CLI enters command mode (the default mode).
 */
static bool cli_subcmd_mode = false;

/**
 * A buffer into which command inputs can be written is declared here, rather
 * than in the command console implementation, to allow multiple command consoles
 * to share the same buffer.
 */
static char *cli_input_buf = NULL;

/**
 * A buffer into which command outputs can be written is declared here, rather
 * than in the command console implementation, to allow multiple command consoles
 * to share the same buffer.
 */
static char *cli_output_buf = NULL;

bool cli_help_set(const T_CLI_CMD *cmd_list, const char *cmd_str, char *buf, size_t buf_len)
{
    static const T_CLI_CMD *p_cli_cmd = NULL;
    const char             *p_param = NULL;
    uint32_t                param_len;
    bool                    ret;

    if (cmd_list == NULL)
    {
        snprintf(buf, buf_len, "Invalid command list entry for traversing.\r\n");
        return false;
    }

    /* Obtain the specified cmd string that needs to display. */
    p_param = cli_param_get(cmd_str, 1, &param_len);

    /* Display all available commands. */
    if (p_param == NULL)
    {
        if (p_cli_cmd == NULL)
        {
            /* Reset back to the start of the list. */
            p_cli_cmd = cmd_list;
        }

        strncpy(buf, p_cli_cmd->p_help, buf_len);

        p_cli_cmd = p_cli_cmd->p_next;
        if (p_cli_cmd == NULL)
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
        for (p_cli_cmd = cmd_list; p_cli_cmd != NULL; p_cli_cmd = p_cli_cmd->p_next)
        {
            if (!strcmp(p_param, p_cli_cmd->p_cmd))
            {
                strncpy(buf, p_cli_cmd->p_help, buf_len);
                break;
            }
        }

        if (p_cli_cmd != NULL)
        {
            /* Found the specific cmd. */
            p_cli_cmd = NULL;
        }
        else
        {
            /* The specific cmd was not supported. */
            snprintf(buf, buf_len,
                     "Incorrect command '%s'. Enter 'help' to view available commands.\r\n",
                     p_param);
        }

        ret = false;
    }

    return ret;
}

static bool cli_help(const char *cmd_str, char *buf, size_t buf_len)
{
    return cli_help_set(p_cli_cmd_list, cmd_str, buf, buf_len);
}

static bool cli_exit(const char *cmd_str, char *buf, size_t buf_len)
{
    (void)cmd_str;
    (void)buf_len;

    buf[0] = '\0';

    return false;
}

static bool cli_mode(const char *cmd_str, char *buf, size_t buf_len)
{
    const char  *p_param = NULL;
    uint32_t     param_len;

    /* Obtain the specified cmd string that needs to display. */
    p_param = cli_param_get(cmd_str, 1, &param_len);
    if (!strcmp(p_param, "string"))
    {
        console_set_mode(CONSOLE_MODE_STRING);
    }
    else if (!strcmp(p_param, "binary"))
    {
        console_set_mode(CONSOLE_MODE_BINARY);
    }
    else
    {
        goto err;
    }

    snprintf(buf, buf_len, "Console enters %s mode.\r\n", p_param);
    return false;

err:
    snprintf(buf, buf_len, "Invalid param %s (string or binary).\r\n", p_param);
    return false;
}

bool cli_cmd_register(T_CLI_CMD *const cli_cmd)
{
    uint32_t s;
    bool ret = false;

    if (cli_cmd != NULL)
    {
        s = os_lock();
        {
            cli_cmd->p_next = NULL;
            p_last_cli_cmd->p_next = cli_cmd;
            p_last_cli_cmd = cli_cmd;
        }
        os_unlock(s);

        ret = true;
    }

    return ret;
}

bool cli_init(uint32_t input_buf_size, uint32_t output_buf_size)
{
    cli_input_buf = os_mem_alloc2(input_buf_size);
    if (cli_input_buf == NULL)
    {
        goto err_input;
    }

    cli_output_buf = os_mem_alloc2(output_buf_size);
    if (cli_output_buf == NULL)
    {
        goto err_output;
    }

    snprintf(cli_prompt, CLI_PROMPT_SIZE,
             CLI_USER CLI_AT CLI_HOST CLI_COLON CLI_DIRECTORY CLI_BANG CLI_SPACE);

    return true;

err_output:
    os_mem_free(cli_input_buf);
err_input:
    return false;
}

char *cli_input_buf_get(void)
{
    return cli_input_buf;
}

char *cli_output_buf_get(void)
{
    return cli_output_buf;
}

bool cli_cmd_process(const char *cmd_str, char *output_buf, size_t buf_len)
{
    static const T_CLI_CMD *p_cli_cmd = NULL;
    size_t                  cmd_len;
    uint32_t                len;
    bool                    ret = true;

    cmd_str = cli_param_get(cmd_str, 0, &len);

    /* Sanity check if command existed. */
    if (cmd_str == NULL)
    {
        output_buf[0] = '\0';
        return false;
    }

    if (p_cli_cmd == NULL)
    {
        if (cli_subcmd_mode == false)
        {
            p_cli_cmd = p_cli_cmd_list;
        }
        else
        {
            p_cli_cmd = p_cli_subcmd_list;
        }

        /* Search for the command string in the list of registered (sub)commands. */
        for (; p_cli_cmd != NULL; p_cli_cmd = p_cli_cmd->p_next)
        {
            if (p_cli_cmd->p_match == NULL)
            {
                cmd_len = strlen(p_cli_cmd->p_cmd);
                if (cmd_len == len && !strncmp(cmd_str, p_cli_cmd->p_cmd, len))
                {
                    /* Check the expected number of parameters. If param_num is -1,
                    * there could be a variable number of parameters.
                    */
                    if (p_cli_cmd->param_num >= 0)
                    {
                        if (cli_param_num_get(cmd_str) != p_cli_cmd->param_num)
                        {
                            ret = false;
                        }
                    }

                    break;
                }
            }
            else
            {
                if (p_cli_cmd->p_match(cmd_str, p_cli_cmd->p_cmd, len) == true)
                {
                    /* Check the expected number of parameters. If param_num is -1,
                    * there could be a variable number of parameters.
                    */
                    if (p_cli_cmd->param_num >= 0)
                    {
                        if (cli_param_num_get(cmd_str) != p_cli_cmd->param_num)
                        {
                            ret = false;
                        }
                    }

                    break;
                }
            }
        }
    }

    if (p_cli_cmd == NULL)
    {
        /* The command was not found. */
        snprintf(output_buf, buf_len,
                 "Invalid command '%s'. Enter 'help' to view available commands.\r\n",
                 cmd_str);

        ret = false;
    }
    else
    {
        if (ret == false)
        {
            /* The command parameter num was incorrect. */
            snprintf(output_buf, buf_len,
                     "Incorrect command parameter(s). Enter 'help %s' to view correct format.\r\n",
                     p_cli_cmd->p_cmd);

            p_cli_cmd = NULL;
        }
        else
        {
            /* Call the callback function that is registered to this command. */
            ret = p_cli_cmd->p_callback(cmd_str, output_buf, buf_len);
            if (ret == false)
            {
                if (cli_subcmd_mode == false && p_cli_cmd->p_next_sub != NULL)
                {
                    /* Enter subcommand mode of the current command. */
                    cli_subcmd_mode = true;
                    p_cli_subcmd_list = p_cli_cmd->p_next_sub;

                    /* Save current command prompt. */
                    snprintf(cli_cached_prompt, CLI_PROMPT_SIZE, "%s", cli_prompt);

                    /* Update the prompt of subcommand mode. */
                    memset(cli_prompt, 0, CLI_PROMPT_SIZE);
                    snprintf(cli_prompt, CLI_PROMPT_SIZE - 2,
                             CLI_USER CLI_AT CLI_HOST CLI_COLON "%s", p_cli_cmd->p_cmd);
                    snprintf(&cli_prompt[strlen(cli_prompt)], 3, "%s", CLI_BANG CLI_SPACE);
                }
                else if (cli_subcmd_mode == true && !strcmp(p_cli_cmd->p_cmd, "exit"))
                {
                    /* Return from subcommand to command mode. */
                    cli_subcmd_mode = false;
                    p_cli_subcmd_list = NULL;
                    snprintf(cli_prompt, CLI_PROMPT_SIZE, "%s", cli_cached_prompt);
                }

                /* Clear p_cli_cmd if the command completed and no more string returned. */
                p_cli_cmd = NULL;
            }
        }
    }

    return ret;
}

const char *cli_param_get(const char *cmd_str, uint32_t param_idx, uint32_t *param_len)
{
    const char *p_param = NULL;
    uint32_t    i = 0;

    *param_len = 0;

    /* Sanity check the cmd_str. */
    if (cmd_str == NULL)
    {
        return p_param;
    }

    while (i <= param_idx)
    {
        /* Skip prefix or internal spaces. */
        while (*cmd_str == '\t' || *cmd_str == ' ')
        {
            cmd_str++;
        }

        /* No parameters left. */
        if (*cmd_str == '\0')
        {
            p_param = NULL;
            break;
        }

        /* Reset parameters. */
        p_param     = cmd_str;
        *param_len  = 0;

        while (*cmd_str != '\0' && *cmd_str != '\t' && *cmd_str != ' ')
        {
            (*param_len)++;
            cmd_str++;
        }

        i++;
    }

    return p_param;
}

int32_t cli_param_num_get(const char *cmd_str)
{
    int32_t param_num  = 0;
    bool    space_flag = false;

    /* Count the number of space delimited param in p_cmd_str. */
    while (*cmd_str != 0x00)
    {
        if (*cmd_str == ' ')
        {
            if (space_flag != true)
            {
                param_num++;
                space_flag = true;
            }
        }
        else
        {
            space_flag = false;
        }

        cmd_str++;
    }

    /* If the command string ended with spaces, then there will have
     * been too many parameters counted.
     */
    if (space_flag == true)
    {
        param_num--;
    }

    /* The value returned is one less than the number of space delimited
     * words, as the first word should be the command itself.
     */
    return param_num;
}

bool cli_prompt_echo(void)
{
    if (strlen(cli_prompt) != 0)
    {
        console_write((uint8_t *)cli_prompt, strlen(cli_prompt));
        return true;
    }

    return false;
}
