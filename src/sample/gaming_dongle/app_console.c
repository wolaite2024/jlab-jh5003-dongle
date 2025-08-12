
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include "board.h"
#include "app_console.h"
#include "app_cfg.h"
#include "app_io_msg.h"
#include "app_flags.h"
#include "console_uart.h"
#include "cli_power.h"
#include "app_timer.h"
#include "app_dlps.h"
#if F_APP_CLI_CFG_SUPPORT
#include "cli_cfg.h"
#endif
#if F_APP_CLI_STRING_MP_SUPPORT
#include "cli_mp.h"
#endif

#define CONSOLE_TX_BUFFER_LARGE         512
#define CONSOLE_RX_BUFFER_LARGE         512
#define CONSOLE_TX_BUFFER_SMALL         256
#define CONSOLE_RX_BUFFER_SMALL         256

#define CONSOLE_UART_DATA_TIMER         10000

#define CONSOLE_UART_RX_BUFFER_SIZE     RX_GDMA_BUFFER_SIZE

static uint8_t timer_idx_console_uart_data_wake_up = 0;
static uint8_t console_uart_date_timer_id = 0;

typedef enum
{
    APP_CONSOLE_UART_DATA_WAKE_UP
} T_APP_CONSOLE_UART_DATA_TIMER;

static void app_console_handle_wake_up(T_CONSOLE_UART_EVENT event)
{
    T_IO_MSG dlps_msg;

    dlps_msg.type = IO_MSG_TYPE_GPIO;
    dlps_msg.subtype = IO_MSG_GPIO_UART_WAKE_UP;

    /* Send MSG to APP task */
    if (!app_io_msg_send(&dlps_msg))
    {
        APP_PRINT_ERROR0("app_dlps_exit_callback: Send DLPS msg error");
    }
}

void app_console_uart_handle_msg(T_IO_MSG *io_driver_msg_recv)
{
    app_dlps_disable(APP_DLPS_ENTER_CHECK_UART_RX);
    app_start_timer(&timer_idx_console_uart_data_wake_up, "console_uart_data_wake_up",
                    console_uart_date_timer_id,
                    APP_CONSOLE_UART_DATA_WAKE_UP, 0, false, CONSOLE_UART_DATA_TIMER);
}

static void app_console_uart_data_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_console_uart_data_timeout_cb: timer_id %d, timer_chann %d", timer_evt, param);
    switch (timer_evt)
    {
    case APP_CONSOLE_UART_DATA_WAKE_UP:
        {
            app_stop_timer(&timer_idx_console_uart_data_wake_up);
            app_dlps_enable(APP_DLPS_ENTER_CHECK_UART_RX);
        }
        break;

    default:
        break;
    }
}

static void app_console_uart_init(void)
{
#if F_APP_CONSOLE_SUPPORT
    T_CONSOLE_PARAM console_param;
    T_CONSOLE_OP    console_op;
    T_CONSOLE_UART_CONFIG console_uart_config;

    console_param.tx_buf_size   = CONSOLE_TX_BUFFER_LARGE;
    console_param.rx_buf_size   = CONSOLE_RX_BUFFER_LARGE;
    console_param.tx_wakeup_pin = app_cfg_const.tx_wake_up_pinmux;
    console_param.rx_wakeup_pin = app_cfg_const.rx_wake_up_pinmux;

    console_op.init = console_uart_init;
    console_op.tx_wakeup_enable = NULL; //console_uart_tx_wakeup_enable
    console_op.rx_wakeup_enable = NULL; //console_uart_rx_wakeup_enable;
    console_op.write = console_uart_write;
    console_op.wakeup = console_uart_wakeup;
    console_op.tx_empty = NULL;

    console_uart_config.one_wire_uart_support = 0;
    console_uart_config.uart_tx_pinmux = app_cfg_const.data_uart_tx_pinmux;
    console_uart_config.uart_rx_pinmux = app_cfg_const.data_uart_rx_pinmux;
    console_uart_config.rx_wake_up_pinmux = app_cfg_const.rx_wake_up_pinmux;
    console_uart_config.enable_rx_wake_up = app_cfg_const.enable_rx_wake_up;
    console_uart_config.data_uart_baud_rate = app_cfg_const.data_uart_baud_rate;
    console_uart_config.callback = app_console_handle_wake_up;

    console_uart_config.uart_dma_rx_buffer_size = CONSOLE_UART_RX_BUFFER_SIZE;

#if (TARGET_RTL8773DO == 1 || TARGET_RTL8773DFL == 1)
    console_uart_config.uart_rx_dma_enable = true;
#else
    console_uart_config.uart_rx_dma_enable = false;
#endif
    console_uart_config.uart_tx_dma_enable = false;

    console_uart_config_init(&console_uart_config);
    console_init(&console_param, &console_op);
#endif
    app_timer_reg_cb(app_console_uart_data_timeout_cb, &console_uart_date_timer_id);
}

void app_console_init(void)
{
#if F_APP_CONSOLE_SUPPORT
    app_console_uart_init();
    power_cmd_register();
#if F_APP_CLI_CFG_SUPPORT
    cfg_cmd_register();
#endif
#if F_APP_CLI_STRING_MP_SUPPORT
    mp_cmd_str_register();
#endif
#endif
}
