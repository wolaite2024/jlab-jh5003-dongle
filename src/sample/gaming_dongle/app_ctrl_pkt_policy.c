#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "trace.h"
#include "gap_br.h"
#include "btm.h"
#include "app_timer.h"
#include "app_ctrl_pkt_policy.h"
#include "app_src_policy.h"
#include "app_gaming_sync.h"
#include "app_dongle_transmit_client.h"
#include "gaming_bt.h"
#include "app_link_util.h"
#include "app_audio_pipe_mgr.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_cfg.h"
#include "app_adapter_service.h"
#include "app_le_audio.h"
#endif
#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_upstream_decode.h"
#include "app_downstream_encode.h"
#endif


#define FIX_CHANNEL_DISALLOW_SNIFF_TIME     5000
#define FIX_CHANNEL_SNIFF_DETECT_TIME       (FIX_CHANNEL_DISALLOW_SNIFF_TIME - 1000)

static uint8_t timer_idx_exit_sniff_send_fix_chann_data = 0;
static uint8_t app_gaming_ctrl_timer_id = 0;

enum
{
    APP_TIMER_EXIT_SNIFF_SEND_FIX_CHAN_DATA,
};

void app_gaming_ctrl_data_rcv(uint8_t *data, uint16_t len)
{
    APP_PRINT_TRACE1("app_gaming_ctrl_data_rcv: len %d", len);
}

bool app_gaming_ctrl_check_timer_need_to_restart(uint32_t curr_time, uint32_t last_start_time,
                                                 uint32_t detect_time)
{
    bool restart_timer = false;

    /* check restart timer to reduce sys loading (reduce the frequency to call app_start_timer)*/
    if (curr_time > last_start_time)
    {
        if ((curr_time - last_start_time) > detect_time)
        {
            restart_timer = true;
        }
    }
    else
    {
        if ((0xffffffff - last_start_time + curr_time) > detect_time)
        {
            restart_timer = true;
        }
    }

    return restart_timer;
}

bool app_gaming_ctrl_send_fix_channel_data(uint8_t *addr, uint16_t cid, uint8_t *p_data,
                                           uint16_t length, bool flushable)
{
    bool ret = false;
    bool restart_timer = false;
    static uint32_t last_start_time = 0;
    uint32_t curr_time = sys_timestamp_get();

    if (timer_idx_exit_sniff_send_fix_chann_data == 0)
    {
        last_start_time = 0;
    }

    if (app_gaming_ctrl_check_timer_need_to_restart(curr_time, last_start_time,
                                                    FIX_CHANNEL_SNIFF_DETECT_TIME))
    {
        restart_timer = true;
    }

    if (restart_timer)
    {
        bt_sniff_mode_disable(addr);

        app_start_timer(&timer_idx_exit_sniff_send_fix_chann_data, "exit_sniff_send_fix_chann_data",
                        app_gaming_ctrl_timer_id, APP_TIMER_EXIT_SNIFF_SEND_FIX_CHAN_DATA, 0, false,
                        FIX_CHANNEL_DISALLOW_SNIFF_TIME);

        last_start_time = curr_time;
    }

    if (gap_br_send_fix_chann_data(addr, cid, p_data, length, flushable) == GAP_CAUSE_SUCCESS)
    {
        ret = true;
    }

    return ret;
}

bool app_gaming_ctrl_data_send_to_earbuds(uint8_t *data, uint16_t size, T_EARBUD_SIDE bud_side)
{
    bool ret = false;

    ret = app_cmd_send_by_le(DONGLE_CMD_CTRL_RAW_DATA, data, size, bud_side);

    APP_PRINT_TRACE2("app_gaming_ctrl_data_send_to_earbuds: size %d ret %d", size, ret);

    return ret;
}

bool app_gaming_ctrl_data_send_to_headset(uint8_t *data, uint16_t size, bool flushable)
{

#if TARGET_LEGACY_GAMING_DONGLE
    uint8_t addr[6] = {0};

    if (app_get_connected_br_link(addr) == NULL)
    {
        APP_PRINT_ERROR0("app_gaming_ctrl_data_send_to_headset: device not connected");
        return false;
    }
#endif

    return app_gaming_sync_data_send(data, size, flushable);
}

static void app_gaming_ctrl_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    switch (timer_evt)
    {
    case APP_TIMER_EXIT_SNIFF_SEND_FIX_CHAN_DATA:
        {
            uint8_t addr[6] = {0};

            app_stop_timer(&timer_idx_exit_sniff_send_fix_chann_data);

            if (app_get_connected_br_link(addr) != NULL)
            {
                bt_sniff_mode_enable(addr, 784, 816, 0, 0);
            }
        }
        break;

    default:
        break;
    }
}

void app_gaming_handle_headset_status(T_HEADSET_STATUS *headset_status)
{
    APP_PRINT_TRACE1("app_gaming_handle_headset_status: %b", TRACE_BINARY(sizeof(T_HEADSET_STATUS),
                                                                          headset_status));

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    static T_LC3PLUS_ENCODE_MODE prev_mode;

    if (adapter_ble_is_connected() &&
        (headset_status->lc3plus_mode != prev_mode))
    {
        app_le_audio_restart_stream();

        prev_mode = headset_status->lc3plus_mode;
    }

    T_ADAPTER_LINK_EVENT event = headset_status->conn_status == HEADSET_PHONE_CONNECTED ?
                                 ADAPTER_EVENT_HEADSET_PHONE_CONNECTED : ADAPTER_EVENT_HEADSET_PHONE_DISCONNECTED;

    adapter_dual_mode_link_mgr(event);
#endif

#if TARGET_LEGACY_GAMING_DONGLE || F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT

#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
    uint8_t *addr = app_cfg_nv.public_bud_addr;
#else
    uint8_t *addr = sink_dev_list[0].remote_info.bd_addr;
#endif
    bool send_big_frame = false;

    src_handle_headset_linkback_tpoll();

    if (headset_status->audio_mixing_support)
    {
        if (headset_status->phone_status != PHONE_STREAM_IDLE)
        {
            send_big_frame = true;
        }
    }

    if (headset_status->increase_a2dp_interval)
    {
        send_big_frame = true;
    }

    if (send_big_frame)
    {
        app_usb_ds_set_sbc_frame_num(GAMING_SBC_MAX_FRAME_NUM);
        app_usb_ds_set_lc3_frame_num(GAMING_LC3_MAX_FRAME_NUM);
        gap_br_cfg_acl_link_flush_tout(addr, AVDTP_STREAM_LONG_DATA_FLUSH_TIMEOUT);
    }
    else
    {
        app_usb_ds_set_sbc_frame_num(GAMING_SBC_FRAME_NUM);
        app_usb_ds_set_lc3_frame_num(GAMING_LC3_FRAME_NUM);
        gap_br_cfg_acl_link_flush_tout(addr, AVDTP_STREAM_DATA_FLUSH_TIMEOUT);
    }

    if (headset_status->enable_3M || send_big_frame)
    {
        src_handle_pkt_type(addr, true);
    }
    else
    {
        src_handle_pkt_type(addr, false);
    }

    if (headset_status->upstream_enable)
    {
        upstream_pipe_create(headset_status->upstream_codec);
    }
#endif

}

void app_gaming_sync_dongle_status(void)
{
    uint8_t addr[6] = {0};

    if (app_get_connected_br_link(addr) != NULL)
    {
        app_spp_cmd_send(0, DONGLE_CMD_SYNC_STATUS, (uint8_t *)&dongle_status, sizeof(dongle_status));
    }
    else
    {
        app_cmd_send_by_le(DONGLE_CMD_SYNC_STATUS, (uint8_t *)&dongle_status, sizeof(T_DONGLE_STATUS),
                           EARBUD_SIDE_ANY);
    }

}

void app_gaming_ctrl_init(void)
{
    app_timer_reg_cb(app_gaming_ctrl_timeout_cb, &app_gaming_ctrl_timer_id);

#if F_APP_GAMING_CONTROLLER_SUPPORT || TARGET_LE_AUDIO_GAMING_DONGLE
    app_gaming_sync_init(false, 256);
#else
    app_gaming_sync_init(true, 128);
#endif

}

