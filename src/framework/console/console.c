/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "os_sync.h"
#include "os_mem.h"
#include "os_task.h"
#include "trace.h"
#include "console.h"
#include "cli_core.h"

#include "section.h"


#define CONSOLE_STATE_IDLE      0
#define CONSOLE_STATE_INIT      1
#define CONSOLE_STATE_ACTIVE    2

#define CONSOLE_SIGNAL_BIT_RX_IND   (1 << 0)
#define CONSOLE_SIGNAL_BIT_TX_START (1 << 1)
#define CONSOLE_SIGNAL_BIT_TX_STOP  (1 << 2)

#define CONSOLE_TASK_NAME       "console"
#define CONSOLE_TASK_PRIORITY   3

typedef struct t_console
{
    uint8_t            *tx_buf_addr;
    uint32_t            tx_buf_size;
    uint32_t            tx_buf_count;
    uint32_t            tx_write_idx;
    uint32_t            tx_read_idx;
    uint8_t            *rx_buf_addr;
    uint32_t            rx_buf_size;
    uint32_t            rx_buf_count;
    uint32_t            rx_write_idx;
    uint32_t            rx_read_idx;
    uint8_t             tx_wakeup_pin;
    uint8_t             rx_wakeup_pin;
    bool                tx_lock;
    T_CONSOLE_MODE      mode;
    uint8_t             state;
    T_CONSOLE_OP        op;
    P_CONSOLE_PARSER    parser;
    void               *task_handle;
} T_CONSOLE;

static T_CONSOLE console;

void console_put_string(const char *p_str, uint32_t str_len)
{
    console_write((uint8_t *)p_str, str_len);
}

bool console_get_string(char *p_str, uint32_t str_len)
{
    return false;
}

bool console_get_char(char *chr)
{
    bool ret = false;
    uint32_t s;

    s = os_lock();

    if (console.rx_buf_count)
    {
        *chr = console.rx_buf_addr[console.rx_read_idx];
        console.rx_read_idx++;
        if (console.rx_read_idx == console.rx_buf_size)
        {
            console.rx_read_idx = 0;
        }
        console.rx_buf_count--;

        ret = true;
    }

    os_unlock(s);
    /* underflow */
    return ret;
}

bool console_put_char(const char chr)
{
    bool     ret = false;
    uint32_t s;

    s = os_lock();

    if (console.tx_buf_size > console.tx_buf_count)
    {
        console.tx_buf_addr[console.tx_write_idx] = (uint8_t)chr;
        console.tx_write_idx++;
        if (console.tx_write_idx == console.tx_buf_size)
        {
            console.tx_write_idx = 0;
        }
        console.tx_buf_count++;

        ret = os_task_signal_send(console.task_handle, CONSOLE_SIGNAL_BIT_TX_START);
    }

    os_unlock(s);

    return ret;
}

void console_string_handle(void)
{
    static uint32_t  input_idx = 0;
    char            *input_buf;
    char             input_char;
    char            *output_buf;
    bool             ret;

    /* Obtain the address of the input/output buffers. It is assumed
     * only one command console interface will be used at any one time.
     */
    input_buf = cli_input_buf_get();
    output_buf = cli_output_buf_get();

    while (console_get_char(&input_char) == true)
    {
        /* Erase the last character in the string. */
        if (input_char == CLI_ASCII_BS || input_char == CLI_ASCII_DEL)
        {
            if (input_idx > 0)
            {
                input_idx--;
                input_buf[input_idx] = '\0';

                console_put_char(CLI_ASCII_BS);
                console_put_char(CLI_ASCII_SPACE);
                console_put_char(CLI_ASCII_BS);
            }

            continue;
        }

        /* Echo the character back. */
        console_put_char(input_char);

        /* Clear the input buf, if EOF (ctrl-c) was encountered. */
        if (input_char == CLI_ASCII_EOF)
        {
            input_idx = 0;
            memset(input_buf, 00, CLI_MAX_INPUT_SIZE);

            /* Echo CTRL-C. */
            console_put_string(CLI_CTRL_C, strlen(CLI_CTRL_C));

            /* Space the output from the input. */
            console_put_string(CLI_NEW_LINE, strlen(CLI_NEW_LINE));

            cli_prompt_echo();
            continue;
        }

        /* The end of the line. */
        if (input_char == '\n' || input_char == '\r')
        {
            /* Space the output from the input. */
            console_put_string(CLI_NEW_LINE, strlen(CLI_NEW_LINE));

            /* If the command is empty, skip to handle next command. */
            if (input_idx == 0)
            {
                cli_prompt_echo();
                continue;
            }

            /* The command interpreter is called repeatedly until it
             * returns false (indicating no more output) as it might
             * generate more than one string.
             */
            do
            {
                /* Get the next output string from the command interpreter. */
                ret = cli_cmd_process(input_buf, output_buf, CLI_MAX_OUTPUT_SIZE);

                /* Write the generated string to console. */
                console_put_string(output_buf, strlen(output_buf));
            }
            while (ret == true);

            /* Prefix new command prompt. */
            cli_prompt_echo();

            /* Clear the input string to receive the next command. */
            input_idx = 0;
            memset(input_buf, 00, CLI_MAX_INPUT_SIZE);
        }
        else
        {
            /* A valid character was entered. When a '\n' is entered, the
             * complete string will be passed to the command interpreter.
             */
            if (input_char >= ' ' && input_char <= '~')
            {
                if (input_idx < CLI_MAX_INPUT_SIZE - 1)
                {
                    input_buf[input_idx++] = input_char;
                    input_buf[input_idx] = '\0';
                }
            }
        }
    }
}

bool console_binary_handle(void)
{
    bool ret = false;
    uint32_t s;

    s = os_lock();

    if (console.rx_buf_count != 0)
    {
        if (console.rx_read_idx + console.rx_buf_count <= console.rx_buf_size)
        {
            if (console.parser != NULL)
            {
                console.parser(console.rx_buf_addr + console.rx_read_idx, console.rx_buf_count);
            }
            console.rx_read_idx += console.rx_buf_count;
            if (console.rx_read_idx == console.rx_buf_size)
            {
                console.rx_read_idx = 0;
            }
        }
        else
        {
            uint32_t temp;

            temp = console.rx_buf_size - console.rx_read_idx;

            if (console.parser != NULL)
            {
                console.parser(console.rx_buf_addr + console.rx_read_idx, temp);
                console.parser(console.rx_buf_addr, console.rx_buf_count - temp);
            }
            console.rx_read_idx = console.rx_buf_count - temp;
        }

        console.rx_buf_count = 0;

        ret = true;
    }

    os_unlock(s);

    return ret;
}

bool console_xmit_handle(void)
{
    uint32_t tx_buf_count;
    uint32_t tx_read_idx;

    tx_buf_count = console.tx_buf_count;
    tx_read_idx  = console.tx_read_idx;

    CONSOLE_PRINT_INFO3("console_xmit_handle: tx_buf_count 0x%08x, tx_read_idx %d, console.tx_buf_size %d",
                        tx_buf_count, tx_read_idx, console.tx_buf_size);
    if (tx_buf_count)
    {
        if (tx_read_idx + tx_buf_count > console.tx_buf_size)
        {
            console.op.write(&console.tx_buf_addr[tx_read_idx], console.tx_buf_size - tx_read_idx);
        }
        else
        {
            console.op.write(&console.tx_buf_addr[tx_read_idx], tx_buf_count);
        }

        return true;
    }
    else if (console.op.tx_empty != NULL)
    {
        console.op.tx_empty();
    }

    return false;
}

void console_task(void *param)
{
    uint32_t signal;

    console.state = CONSOLE_STATE_ACTIVE;

    cli_init(CLI_MAX_INPUT_SIZE, CLI_MAX_OUTPUT_SIZE);

    if (console.mode == CONSOLE_MODE_STRING)
    {
        /* Send cli welcome message. */
        console_put_string(CLI_WELCOME_MESSAGE, strlen(CLI_WELCOME_MESSAGE));

        /* Prefix command line prompt. */
        cli_prompt_echo();
    }

    while (true)
    {
        signal = 0;

        os_task_signal_recv(&signal, 0xFFFFFFFFUL);

        CONSOLE_PRINT_INFO5("console_task: signal 0x%08x, rx_write_idx %d, rx_read_idx %d, tx_write_idx %d, tx_read_idx %d",
                            signal, console.rx_write_idx, console.rx_read_idx, console.tx_write_idx, console.tx_read_idx);

        if (signal & CONSOLE_SIGNAL_BIT_RX_IND)
        {
            if (console.mode == CONSOLE_MODE_STRING)
            {
                console_string_handle();
            }
            else
            {
                console_binary_handle();
            }
        }

        if (signal & CONSOLE_SIGNAL_BIT_TX_START)
        {
            if (console.tx_lock == false)
            {
                console.tx_lock = console_xmit_handle();
            }
        }

        if (signal & CONSOLE_SIGNAL_BIT_TX_STOP)
        {
            console.tx_lock = console_xmit_handle();
        }
    }
}

RAM_TEXT_SECTION
bool console_callback(T_CONSOLE_EVT evt, uint8_t *buf, uint32_t len)
{
    bool ret = true;

    switch (evt)
    {
    case CONSOLE_EVT_OPENED:
        console.state = CONSOLE_STATE_INIT;
        break;

    case CONSOLE_EVT_DATA_IND:
        if (console.state == CONSOLE_STATE_ACTIVE)
        {
            if (len <= console.rx_buf_size)
            {
                /* TODO check ring buffer overflow */
                if (console.rx_write_idx + len <= console.rx_buf_size)
                {
                    memcpy(&console.rx_buf_addr[console.rx_write_idx], buf, len);
                    console.rx_buf_count += len;
                    console.rx_write_idx += len;
                    if (console.rx_write_idx == console.rx_buf_size)
                    {
                        console.rx_write_idx = 0;
                    }
                }
                else
                {
                    uint32_t temp;

                    temp = console.rx_buf_size - console.rx_write_idx;
                    memcpy(&console.rx_buf_addr[console.rx_write_idx], buf, temp);
                    memcpy(&console.rx_buf_addr[0], &buf[temp], len - temp);
                    console.rx_buf_count += len;
                    console.rx_write_idx  = len - temp;
                }

                ret = os_task_signal_send(console.task_handle, CONSOLE_SIGNAL_BIT_RX_IND);
            }
        }
        break;

    case CONSOLE_EVT_DATA_XMIT:
        console.tx_buf_count -= len;
        console.tx_read_idx  += len;
        if (console.tx_read_idx == console.tx_buf_size)
        {
            console.tx_read_idx = 0;
        }

        ret = os_task_signal_send(console.task_handle, CONSOLE_SIGNAL_BIT_TX_STOP);
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}

bool console_init(T_CONSOLE_PARAM *console_param, T_CONSOLE_OP *console_op)
{
    int32_t ret = 0;

    console.tx_buf_size     = console_param->tx_buf_size;
    console.rx_buf_size     = console_param->rx_buf_size;
    console.tx_buf_count    = 0;
    console.rx_buf_count    = 0;
    console.tx_write_idx    = 0;
    console.rx_write_idx    = 0;
    console.tx_read_idx     = 0;
    console.rx_read_idx     = 0;
    console.tx_wakeup_pin   = console_param->tx_wakeup_pin;
    console.rx_wakeup_pin   = console_param->rx_wakeup_pin;
    console.tx_lock         = false;
    console.mode            = CONSOLE_MODE_STRING;
    console.state           = CONSOLE_STATE_IDLE;
    console.parser          = NULL;

    console.op.init             = console_op->init;
    console.op.tx_wakeup_enable = console_op->tx_wakeup_enable;
    console.op.rx_wakeup_enable = console_op->rx_wakeup_enable;
    console.op.write            = console_op->write;
    console.op.wakeup           = console_op->wakeup;
    console.op.tx_empty         = console_op->tx_empty;

    console.tx_buf_addr = os_mem_alloc2(console.tx_buf_size);
    if (console.tx_buf_addr == NULL)
    {
        ret = 1;
        goto err_tx_buf;
    }

    console.rx_buf_addr = os_mem_alloc2(console.rx_buf_size);
    if (console.rx_buf_addr == NULL)
    {
        ret = 2;
        goto err_rx_buf;
    }

    if (console.op.init(console_callback) == false)
    {
        ret = 3;
        goto err_op_init;
    }

    if (console_op->tx_wakeup_enable)
    {
        console.op.tx_wakeup_enable(console.tx_wakeup_pin);
    }

    if (console_op->rx_wakeup_enable)
    {
        console.op.rx_wakeup_enable(console.rx_wakeup_pin);
    }

    if (os_task_create(&console.task_handle, CONSOLE_TASK_NAME, console_task,
                       NULL, 768, CONSOLE_TASK_PRIORITY) == false)
    {
        ret = 4;
        goto err_task_create;
    }

    return true;

err_task_create:
err_op_init:
    os_mem_free(console.rx_buf_addr);
err_rx_buf:
    os_mem_free(console.tx_buf_addr);
err_tx_buf:
    CONSOLE_PRINT_ERROR1("console_init: failed %d", -ret);
    return false;
}

bool console_set_mode(T_CONSOLE_MODE mode)
{
    CONSOLE_PRINT_INFO1("console_set_mode: mode %d", mode);
    console.mode = mode;

    return true;
}

T_CONSOLE_MODE console_get_mode(void)
{
    return console.mode;
}

bool console_register_parser(P_CONSOLE_PARSER parser)
{
    if (console.parser == NULL)
    {
        console.parser = parser;
        return true;
    }

    return false;
}

bool console_unregister_parser(void)
{
    if (console.parser != NULL)
    {
        console.parser = NULL;
        return true;
    }

    return false;
}

bool console_write(uint8_t *buf, uint32_t len)
{
    bool      ret = false;
    uint32_t  s;

    CONSOLE_PRINT_INFO4("console_write: tx_buf_size %d, tx_buf_count %d, tx_write_idx %d, len %d",
                        console.tx_buf_size, console.tx_buf_count, console.tx_write_idx, len);

    /* Discard if console transport not ready. */
    if (console.state == CONSOLE_STATE_ACTIVE)
    {
        /* Sanity check len */
        if (len == 0)
        {
            return false;
        }

        s = os_lock();

        if (console.tx_buf_count + len <= console.tx_buf_size)
        {
            if (console.tx_write_idx + len > console.tx_buf_size)
            {
                uint32_t truncated_len;

                truncated_len = console.tx_buf_size - console.tx_write_idx;
                memcpy(&console.tx_buf_addr[console.tx_write_idx], buf, truncated_len);
                memcpy(&console.tx_buf_addr[0], buf + truncated_len, len - truncated_len);

                console.tx_buf_count += len;
                console.tx_write_idx  = len - truncated_len;
            }
            else
            {
                memcpy(&console.tx_buf_addr[console.tx_write_idx], buf, len);
                console.tx_buf_count += len;
                console.tx_write_idx += len;
                if (console.tx_write_idx == console.tx_buf_size)
                {
                    console.tx_write_idx = 0;
                }
            }

            ret = true;
        }

        os_unlock(s);

        if (ret == true)
        {
            os_task_signal_send(console.task_handle, CONSOLE_SIGNAL_BIT_TX_START);
        }
    }

    return ret;
}

RAM_TEXT_SECTION bool console_wakeup()
{
    if (console.state == CONSOLE_STATE_ACTIVE)
    {
        return console.op.wakeup();
    }

    return false;
}
