
/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "os_sched.h"
#include "os_msg.h"
#include "os_task.h"
#include "os_sched.h"
#include "pm.h"
#if CONFIG_SOC_SERIES_RTL8763E_V2
#include "os_heap.h"
#endif
#include "mem_config.h"
#include "rtl876x.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio_int.h"
#include "hal_gpio.h"
#include "trace.h"
#include "audio.h"
#include "sysm.h"
#include "remote.h"
#include "gap_br.h"
#include "gap.h"
#include "app_console.h"
//#include "bqb.h"
#include "test_mode.h"
#include "single_tone.h"
//#include "engage.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_gap.h"
#include "app_io_msg.h"
#include "app_timer.h"
#include "app_key_process.h"
#include "app_dlps.h"
#include "app_key_gpio.h"
#include "app_sdp.h"
#include "app_device.h"
#include "app_a2dp.h"
#include "app_avrcp.h"
#include "app_spp.h"
#include "app_spp_cmd.h"
#include "app_auto_power_off.h"
#include "app_led_ctl.h"
#include "app_mmi.h"
#include "system_status_api.h"
#include "app_bond.h"
#include "app_io_output.h"

#include "board.h"
#include "app_cli_main.h"

#include "app_usb.h"
#include "app_audio_path.h"

#include "ual_adapter.h"
#include "app_adapter_service.h"
#include "le_audio_service.h"
#include "le_service_mgr.h"
#include "app_usb_layer.h"

#if F_APP_HIFI4_SUPPORT
#include "app_buck_tps62860.h"
#include "ipc.h"
#endif

#if APP_DEBUG_REPORT
#include "app_status_report.h"
#endif
#if UAL_CONSOLE_PRINT
#include "cli_console_cmd.h"
#endif
#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif
#include "app_dongle_vol.h"
#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif

#if UAL_LATENCY_TEST
#include "pm.h"
#endif


#ifdef LEGACY_BT_GENERAL
#include "app_src_a2dp.h"
#include "app_sdp_a2dp_src.h"
#include "app_hfp_ag.h"
#include "app_ctrl_cfg.h"
#include "app_src_asp.h"
#include "app_src_ll.h"
#include "app_system_call.h"
#include "le_bg_conn_mgr.h"
#include "app_usb_call_control.h"
#endif

#ifdef LEGACY_BT_GAMING
#include "legacy_gaming.h"
#include "app_src_policy.h"
#endif

#include "app_audio_pipe_mgr.h"
#include "app_downstream_encode.h"

#include "legacy_audio_wrapper.h"
#include "app_dongle_transmit_client.h"
#include "general_audio_wrapper.h"
#include "le_audio_wrapper.h"
#include "app_cfu.h"
#include "app_spp_audio.h"
#include "pm.h"
#include "gap_br.h"
#include "app_gaming_ctrl_cfg.h"
#include "app_charger.h"
#include "app_gaming_sync.h"
#include "app_ctrl_pkt_policy.h"

#if F_APP_USB_GIP_SUPPORT
#include "app_gip.h"
#include "app_gip_security.h"
#endif

#if TARGET_VIZIER
#include "audio_pad.h"
#endif

#if F_APP_HANDLE_DS_PIPE_IN_ISR
#include "audio_pipe.h"
#endif

#define MAX_NUMBER_OF_GAP_MESSAGE       0x20    //!< indicate BT stack message queue size
#define MAX_NUMBER_OF_IO_MESSAGE        0x80    //!< indicate io queue size, extra 0x20 for data uart
#define MAX_NUMBER_OF_DSP_MSG           0x20    //!< number of dsp message reserved for DSP message handling.
#define MAX_NUMBER_OF_CODEC_MSG         0x20    //!< number of codec message reserved for CODEC message handling.
#define MAX_NUMBER_OF_SYS_MSG           0x20    //!< indicate SYS timer queue size
#define MAX_NUMBER_OF_APP_TIMER_MODULE  0x30    //!< indicate app timer module size
/** indicate rx event queue size*/
#define MAX_NUMBER_OF_RX_EVENT      \
    (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE + MAX_NUMBER_OF_DSP_MSG \
     + MAX_NUMBER_OF_CODEC_MSG + MAX_NUMBER_OF_APP_TIMER_MODULE + MAX_NUMBER_OF_SYS_MSG)

#define DEFAULT_PAGESCAN_WINDOW             0x12
#define DEFAULT_PAGESCAN_INTERVAL           0x800 //0x800
#define DEFAULT_PAGE_TIMEOUT                0x2000
#define DEFAULT_SUPVISIONTIMEOUT            0x1f40 //0x7D00
#define DEFAULT_INQUIRYSCAN_WINDOW          0x12
#define DEFAULT_INQUIRYSCAN_INTERVAL        0x800 //0x1000

void *app_task_handle;

void *audio_evt_queue_handle;
void *audio_io_queue_handle;

T_APP_DB app_db;

extern void key_mfb_init(void);

#if TARGET_RTL8773DO
extern int32_t pm_cpu_freq_set(uint32_t required_mhz, uint32_t *actual_mhz);
#endif


#ifdef APP_LATENCY_MEASURE_PIN_REUSE
static void latency_measure_pin_reuse_init(void)
{
    Pad_Config(TRACK_START_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(TRACK_START_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pad_Config(ENC_DATA_IND_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(ENC_DATA_IND_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pad_Config(ISO_SEND_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(ISO_SEND_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pad_Config(GAP_SEND_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(GAP_SEND_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);
}
#endif
/**
* @brief board_init() contains the initialization of pinmux settings and pad settings.
*
*   All the pinmux settings and pad settings shall be initiated in this function.
*   But if legacy driver is used, the initialization of pinmux setting and pad setting
*   should be peformed with the IO initializing.
*
* @return void
*/
static void board_init(void)
{
    uint32_t freq;
    pm_cpu_freq_set(pm_cpu_max_freq_get(), &freq);
    pm_cpu_set_auto_slow_enable(false);
    //Config DATA UART0 pinmux. For external mcu
    if (app_cfg_const.enable_data_uart)
    {
        Pinmux_Config(app_cfg_const.data_uart_tx_pinmux, UART0_TX);
        Pad_Config(app_cfg_const.data_uart_tx_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);

        Pinmux_Config(app_cfg_const.data_uart_rx_pinmux, UART0_RX);
        Pad_Config(app_cfg_const.data_uart_rx_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

        if (app_cfg_const.enable_rx_wake_up)
        {
            Pinmux_Config(app_cfg_const.rx_wake_up_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.rx_wake_up_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
        }

        if (app_cfg_const.enable_tx_wake_up)
        {
            Pinmux_Config(app_cfg_const.tx_wake_up_pinmux, DWGPIO);
            Pad_Config(app_cfg_const.tx_wake_up_pinmux,
                       PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        }
    }

    //Config  UART3 pinmux. For DSP debug
    if (app_cfg_const.dsp_log_output_select == DSP_OUTPUT_LOG_BY_UART)
    {
        Pinmux_Config(app_cfg_const.dsp_log_pin, UART2_TX);
        Pad_Config(app_cfg_const.dsp_log_pin,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }

#if F_APP_PWM_SUPPORT
    //Config PWM pinmux
    if (app_cfg_const.pwm_support)
    {
        Pinmux_Config(app_cfg_const.pwm_pinmux, TIMER_PWM5);
        Pad_Config(app_cfg_const.pwm_pinmux,
                   PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    }
#endif

#if UAL_LATENCY_TEST
    Pinmux_Config(P0_3, DWGPIO);
    Pad_Config(P0_3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);
#endif

#if MIC_CODEC_DEBUG
    Pad_Config(P0_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P2_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P2_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pinmux_Config(P2_1, BCLK_SPORT0);
    Pinmux_Config(P0_2, SDO_CODEC_SLAVE);
    Pinmux_Config(P2_2, LRC_RX_SPORT0);
#endif

#if F_APP_USB_GIP_SUPPORT
#else
    Pinmux_Config(app_cfg_const.i2c_0_dat_pinmux, I2C0_DAT);
    Pinmux_Config(app_cfg_const.i2c_0_clk_pinmux, I2C0_CLK);

    Pad_Config(app_cfg_const.i2c_0_dat_pinmux,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(app_cfg_const.i2c_0_clk_pinmux,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_PullConfigValue(app_cfg_const.i2c_0_dat_pinmux, PAD_STRONG_PULL);
    Pad_PullConfigValue(app_cfg_const.i2c_0_clk_pinmux, PAD_STRONG_PULL);
#endif
}

static void driver_init(void)
{
    hal_gpio_init();
    hal_gpio_int_init();
    hal_gpio_set_debounce_time(30);
    if (app_cfg_const.key_gpio_support)
    {
        key_gpio_initial();
        app_key_init();
    }

    if (app_cfg_const.charger_support || app_cfg_const.discharger_support)
    {
        app_charger_init();
    }

    if (app_cfg_const.led_support)
    {
        app_led_init();
    }

    if (app_cfg_const.enable_data_uart)
    {
#if F_APP_CONSOLE_SUPPORT
        app_console_init();
#endif
    }

    app_usb_init();
    app_cfg_const.enable_dlps = 1;

#if F_APP_CLI_CFG_SUPPORT
    app_cmd_register();
#endif

#if UAL_CONSOLE_PRINT
    console_cmd_register();
#endif

#if F_APP_HIFI4_SUPPORT
    app_buck_tps62860_init();
#endif

#if F_APP_LINEIN_SUPPORT
    if (app_cfg_const.line_in_support)
    {
        app_line_in_driver_init();
    }
#endif

#if (MFB_KEY_SUPPORT == 1)
    if (!(app_cfg_const.key_enable_mask & KEY0_MASK))
    {
        key_mfb_init();
    }
#endif

}

static void framework_init(void)
{
    /* System Manager */
    sys_mgr_init(audio_evt_queue_handle);

    /* RemoteController Manager */
    remote_mgr_init((T_REMOTE_SESSION_ROLE)app_cfg_nv.bud_role);
    remote_local_addr_set(app_cfg_nv.bud_local_addr);
    remote_peer_addr_set(app_cfg_nv.bud_peer_addr);

    /* Bluetooth Manager */
    bt_mgr_init();

    /* Audio Manager */
    audio_mgr_init(PLAYBACK_POOL_SIZE, VOICE_POOL_SIZE, RECORD_POOL_SIZE, NOTIFICATION_POOL_SIZE);

#if F_APP_HANDLE_DS_PIPE_IN_ISR
    /* Audio Pipe Buf init for handle pipe data in isr */
    audio_pipe_buf_init();
#endif
}

static void app_task(void *pvParameters)
{
    uint8_t event;
    volatile uint32_t start_time;
    volatile uint32_t end_time;
    volatile uint32_t delta_time;

    bt_adap_start_task(audio_evt_queue_handle, audio_io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);
    app_usb_layer_init();
#if DONGLE_LE_AUDIO
    app_le_audio_init();
#endif
    ble_service_mgr_init(audio_evt_queue_handle, audio_io_queue_handle);

#ifdef APP_LATENCY_MEASURE_PIN_REUSE
    //latency_measure_pin_reuse_init();
#endif
    while (true)
    {
        if (os_msg_recv(audio_evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            //DBG_DIRECT("app_task event 0x%x", event);
            if (EVENT_GROUP(event) == EVENT_GROUP_IO)
            {
                T_IO_MSG io_msg;

                if (os_msg_recv(audio_io_queue_handle, &io_msg, 0) == true)
                {
                    if (event == EVENT_IO_TO_APP)
                    {
                        start_time = log_timestamp_get();
                        app_io_handle_msg(io_msg);
                    }
                }
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_STACK)
            {
                start_time = log_timestamp_get();
                gap_handle_msg(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_FRAMEWORK)
            {
                start_time = log_timestamp_get();
                sys_mgr_event_handle(event);
            }
            else if (EVENT_GROUP(event) == EVENT_GROUP_APP)
            {
                app_timer_handle_msg(event);
            }

            //if (app_cfg_const.led_support)
            //{
            //    led_check_mode();
            //}
        }
    }
}

#ifdef F_APP_TEST_CFG
void app_test_cfg(void)
{
    uint8_t device_name_legacy[40] = "bb2_dongle";

    app_cfg_nv.bud_role = 0;
//    app_cfg_const.bud_role = 0;

    memcpy(&app_cfg_nv.device_name_legacy[0], device_name_legacy, 40);

    app_cfg_nv.app_is_power_on = 1;
    app_cfg_nv.factory_reset_done = 1;
    app_cfg_const.supported_profile_mask = A2DP_PROFILE_MASK |
                                           AVRCP_PROFILE_MASK |
                                           SPP_PROFILE_MASK;

    /* enable data uart for console */
    app_cfg_const.data_uart_tx_pinmux = DATA_UART_TX_PIN;
    app_cfg_const.data_uart_rx_pinmux = DATA_UART_RX_PIN;
    app_cfg_const.enable_data_uart = 1;
    app_cfg_const.data_uart_baud_rate = BAUD_RATE_115200;

    /* for bb2 QFN228 evb motherboard */
    app_cfg_const.mfb_replace_key0 = 1;
    app_cfg_const.key_pinmux[1] = P1_3;
    app_cfg_const.key_pinmux[2] = P1_4;
    app_cfg_const.key_enable_mask = 0x06;
    app_cfg_const.key_high_active_mask = 0x00;

    app_cfg_const.discharger_support = 0;

    app_cfg_const.led_0_pinmux = P0_0;
    app_cfg_const.led_1_pinmux = P1_0;
    app_cfg_const.led_2_pinmux = P1_1;
}
#endif

#if (SUPPORT_ACCESS_SHM == 1)
void shm_data_copy(void)
{
#if defined (__CC_ARM)
    extern unsigned int Load$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RW$$Length;
    extern unsigned int Load$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$ZI$$Length;

    uint32_t load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Base;
    uint32_t len = (uint32_t)&Image$$SHARE_RAM_DATA$$RW$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);

    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$ZI$$Length;
    memset((uint8_t *)dest_addr, 0, len);

    extern unsigned int Load$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Base;
    extern unsigned int Image$$SHARE_RAM_DATA$$RO$$Length;

    load_addr = (uint32_t)&Load$$SHARE_RAM_DATA$$RO$$Base;
    dest_addr = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Base;
    len = (uint32_t)&Image$$SHARE_RAM_DATA$$RO$$Length;
    memcpy((uint8_t *)dest_addr, (uint8_t *)load_addr, len);
#elif defined (__GNUC__)
    extern unsigned int *__share_ram_load_addr__;
    extern unsigned int *__share_ram_dst_addr__;
    extern unsigned int *__share_ram_code_length__;
    memcpy((uint32_t)&__share_ram_dst_addr__,
           (uint32_t)&__share_ram_load_addr__,
           (uint32_t)&__share_ram_code_length__);

#endif
}
#endif

void ram_config()
{
#if (CONFIG_SOC_SERIES_RTL8763E_V2 == 1)
    extern void sys_hall_set_dsp_share_memory_80k(bool is_off_ram);
    sys_hall_set_dsp_share_memory_80k(false);
#endif

#if (SUPPORT_ACCESS_SHM == 1)
    heap_shm_init(DSP_SHM_HEAP_ADDR, DSP_SHM_HEAP_SIZE);
    heap_shm_set(DSP_SHM_GLOBAL_ADDR, DSP_SHM_TOTAl_SIZE, 0);
    shm_data_copy();
    add_shm_to_continuous_tail();
#endif

}

int main(void)
{
    APP_PRINT_INFO2("APP COMPILE TIME: [%s - %s]", TRACE_STRING(__DATE__), TRACE_STRING(__TIME__));
    APP_PRINT_INFO3("Dongle APP PID%d, version %d.%d", APP_PID, APP_VERSION_MAJOR,
                    APP_VERSION_MINOR);

#if F_APP_HIFI4_SUPPORT
    void *ipc_task_handle = NULL;
#endif

    ram_config();
    memset(&app_db, 0, sizeof(T_APP_DB));
    app_db.br_link = calloc(1, MAX_BR_LINK_NUM * sizeof(T_APP_BR_LINK));


    if (!app_db.br_link)
    {
        DBG_DIRECT("alloc app_db.br_link failed");
        return -1;
    }

    if (sys_hall_get_reset_status())
    {
        APP_PRINT_INFO0("APP RESTART FROM WDT_SOFTWARE_RESET");
    }
    else
    {
        APP_PRINT_INFO0("APP START FROM HW_RESET");
    }

    if (is_single_tone_test_mode())
    {
        os_msg_queue_create(&audio_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
        os_msg_queue_create(&audio_evt_queue_handle, "evtQ", MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));

        app_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);
        app_cfg_init();
#if (MFB_KEY_SUPPORT == 1)
        if (app_cfg_const.mfb_replace_key0)
        {
            app_cfg_const.key_enable_mask &= 0xfe;
            app_cfg_const.key_pinmux[0] = 0xff;
        }
#endif
        board_init();
        bt_adap_init(adapter_msg_cback);
        bt_adap_set_scan_mdoe(GAP_SCAN_MODE_PASSIVE);
#if (F_APP_SC_KEY_DERIVE_SUPPORT == 1)
        app_ble_key_derive_init();
#endif
#if TX_POWER_CTRL
        bt_pairing_tx_power_set(-2);
#endif
        app_dlps_init();
        framework_init();
        driver_init();
        app_auto_power_off_init();
        reset_single_tone_test_mode();
        mp_hci_test_init(MP_HCI_TEST_DUT_MODE);
#ifdef LEGACY_BT_GAMING
        if (app_cfg_const.led_support)
        {
            app_led_set_mode(APP_LED_MODE_ALL_ON);
        }
#endif
    }
    else
    {
        os_msg_queue_create(&audio_io_queue_handle, "ioQ", MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
        os_msg_queue_create(&audio_evt_queue_handle, "evtQ", MAX_NUMBER_OF_RX_EVENT, sizeof(unsigned char));

#if APP_DEBUG_REPORT
        app_status_report_init();
#endif

        app_init_timer(audio_evt_queue_handle, MAX_NUMBER_OF_APP_TIMER_MODULE);

        app_cfg_init();

#ifdef LEGACY_BT_GAMING
        /* fixed cod for legacy gaming to for headset indentificaiton */
        app_cfg_const.class_of_device[0] = 0x00;
        app_cfg_const.class_of_device[1] = 0x01;
        app_cfg_const.class_of_device[2] = 0x24;
#endif

#if F_APP_USB_HIGH_SPEED_0_5MS
        app_cfg_const.dongle_usb_speed = 1;
#endif

#if F_APP_SUPPORT_CAPTURE_ACOUSTICS_MP
        app_cfg_const.enable_rtk_vendor_cmd = 1;
        app_cfg_const.enable_dsp_capture_data_by_spp = 1;
        app_cfg_const.dt_resend_num = 5; // sync with rws, used by app_transfer_bt_resend
#endif

#ifdef FPGA_DEV_APP_CUSTOM_CFG
        app_cfg_nv.factory_reset_done = 1;
#endif
#ifdef F_APP_TEST_CFG
        //app_test_cfg();
#endif
#if (MFB_KEY_SUPPORT == 1)
        if (app_cfg_const.mfb_replace_key0)
        {
            app_cfg_const.key_enable_mask &= 0xfe;
            app_cfg_const.key_pinmux[0] = 0xff;
        }
#endif

#ifdef ENABLE_SPI0_SLAVE
        /* Leave these pins for SPI0 */
        app_cfg_const.key_enable_mask &= ~KEY2_MASK;
        app_cfg_const.led_0_pinmux = 0xff;
        app_cfg_const.led_2_pinmux = 0xff;
#endif

        //driver_init();
        board_init();

#if UAL_LATENCY_TEST
        log_module_bitmap_trace_set(0xffffffffffffffff, LEVEL_TRACE, false);
        log_module_bitmap_trace_set(0xffffffffffffffff, LEVEL_INFO, false);
        log_module_trace_set(MODULE_APP, LEVEL_TRACE, true);
        log_module_trace_set(MODULE_APP, LEVEL_INFO, true);
#endif
        bt_ual_set_param(GAP_BR_PARAM_COD, sizeof(uint32_t), app_cfg_const.class_of_device);
        bt_ual_set_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_le);
        bt_ual_set_param(GAP_BR_PARAM_NAME, GAP_DEVICE_NAME_LEN, app_cfg_nv.device_name_legacy);

        bt_adap_init(adapter_msg_cback);
        bt_adap_set_scan_mdoe(GAP_SCAN_MODE_PASSIVE);
        le_service_gap_params_reset();
#if TX_POWER_CTRL
        bt_pairing_tx_power_set(-2);
#endif
        app_dlps_init();
        framework_init();

#if TARGET_VIZIER
        audio_pad_spk_pad_cfg(false);
#endif

#if F_APP_USB_GIP_SUPPORT
        app_gip_mode_switch_init();
#endif
        driver_init();

#if F_APP_USB_GIP_SUPPORT
        app_gip_pscc_init();
#endif

        app_auto_power_off_init();
#if F_APP_IO_OUTPUT_SUPPORT
        app_io_output_init();
#endif

        app_gap_init();
        app_avrcp_init();
#ifdef LEGACY_BT_GENERAL
        app_src_a2dp_init();
        app_sdp_a2dp_src_init();
        app_hfp_ag_init();
        app_ctrl_cfg_init();
        app_usb_call_control_init();
#ifdef MICROSOFT_HS_ASP_FEATURE_SUPPORT
        app_src_asp_init();
#endif
        app_src_ll_init();
        //le_bg_conn_mgr_init();
#else
        app_a2dp_init();
        app_spp_init();
        app_spp_cmd_init();
        app_sdp_init();
        gaming_app_ctrl_cfg_init();
#endif
        app_device_init();
        app_mmi_init();
#if LEGACY_GAMING_LOCK_HEADSET_SUPPORT
        app_src_legacy_lock_info_init();
#endif
        app_src_legacy_key_init();
        //app_a2dp_src_key_cfg_init();
        app_dongle_vol_init();

#if F_APP_USB_GIP_SUPPORT
        app_gip_init();
#endif

        app_cfu_init();

#ifdef LEGACY_BT_GAMING
        legacy_audio_init(audio_evt_queue_handle, audio_io_queue_handle);
#else
        general_audio_init(audio_evt_queue_handle, audio_io_queue_handle);
#if TARGET_RTL8773DO
#else
        app_spp_audio_init();
#endif
#endif

#ifdef LEGACY_BT_GAMING
        legacy_gaming_init(src_legacy_gaming_event_cback);
#endif

        le_audio_init(audio_evt_queue_handle, audio_io_queue_handle);

#if DONGLE_TRANSMIT_CLIENT_SUPPORT
        app_dongle_transmit_client_init();
#endif
        app_gaming_ctrl_init();

        app_usb_ds_init();

        app_pipe_init();

#if F_APP_LOCAL_PLAYBACK_SUPPORT
        os_task_create(&app_task_handle, "app_task", app_task, NULL, 1024 * 4, 2);
#else
        os_task_create(&app_task_handle, "app_task", app_task, NULL, 1024 * 3, 2);
#endif

#if F_APP_HIFI4_SUPPORT
        os_task_create(&ipc_task_handle, "ipc_task", ipc_task, NULL, 1024 * 2, 2);
#endif
    }
    os_sched_start();

    return 0;
}

