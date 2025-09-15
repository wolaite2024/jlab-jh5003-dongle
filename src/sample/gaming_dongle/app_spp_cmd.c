/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file      app_spp_cmd.c
  * @brief
  * @details
  * @author
  * @date
  * @version
  ***************************************************************************************
  * @attention
  ***************************************************************************************
  */

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include <stdlib.h>
#include "trace.h"
#include "os_queue.h"
#include "app_mmi.h"
#include "app_cfg.h"
#include "app_spp_cmd.h"
#include "gaming_bt.h"
#ifdef LEGACY_BT_GAMING
#include "app_ctrl_pkt_policy.h"
#include "app_usb_audio_wrapper.h"
#endif
#include "app_spp_audio.h"
#include "app_cfu_passthrough.h"
#include "app_usb_passthrough.h"
#include "app_usb_hid.h"
#include "app_src_policy.h"
#include "app_flags.h"
#include "app_gaming_sync.h"
#if F_APP_GAMING_LEA_A2DP_SWITCH_SUPPORT
#include "app_adapter_service.h"
#include "app_le_audio.h"
#endif
#include "app_usb_vol_control.h"
#include "app_upstream_decode.h"
#include "app_downstream_encode.h"
#include "gap_br.h"
#include "teams_call_control.h"

/*============================================================================*
 *                         Macros
 *============================================================================*/
typedef enum
{
    SPP_CMD_EQ_PT = 0x10,
    SPP_CMD_CFU_PT = 0x11,
} T_SPP_CMD_TYPE;
/*============================================================================*
 *                         Types
 *============================================================================*/


/*============================================================================*
 *                         global variable
 *============================================================================*/


/*============================================================================*
 *                         local variable
 *============================================================================*/


typedef struct __attribute__((packed))
{
    uint8_t sync;
    uint8_t type: 4;
    uint8_t pl_high: 4;
    uint8_t pl;
    uint8_t cmd;
    uint8_t *payload;
    uint8_t sync_end;
} spp_cmd_t;

#define SRC_SPP_HEADER_SIZE 4
/*============================================================================*
 *                         Functions
 *============================================================================*/
#if 0
static void app_spp_cmd_cback(uint8_t *p_data, uint16_t len)
{
    if ((!p_data) || (!len))
    {
        return;
    }
    uint8_t cmd = p_data[0];

    APP_PRINT_INFO2("app_spp_cmd_cback cmd %x, len %d", cmd, len);
    switch (cmd)
    {
    case 0:
        break;

    case DONGLE_CMD_CTRL_RAW_DATA:
        {
            handle_dongle_ctrl_pkt(p_data + 1, len - 1);
        }
        break;
    case 0xFF:
        break;
    default:
        break;
    }
}
#endif

void app_cmd_send_by_spp(uint8_t cmd, uint8_t *data, uint16_t len)
{
    uint8_t *p_data = calloc(1, len + 5);
    if (!p_data)
    {
        return;
    }
    APP_PRINT_INFO2("app_cmd_send_by_spp data %b len %d", TRACE_BINARY(len, data), len);
	APP_PRINT_INFO1("----> cmd 0x%x",cmd);
    spp_cmd_t *p_spp = (spp_cmd_t *)p_data;
    p_spp->sync = 'R';
    p_spp->type = 0x01;
    p_spp->pl = (len + 1) & 0xFF;
    p_spp->pl_high = ((len + 1) >> 8) & 0x0F;
    p_spp->cmd = cmd;
    memcpy(&(p_spp->payload), data, len);
    p_data[len + 4] = 'T';
    /*send out by spp*/
    app_send_out_by_spp(p_data, len + 5);
    free(p_data);
}

bool app_spp_eq_send(uint8_t *data, uint16_t len)
{
    app_cmd_send_by_spp(SPP_CMD_EQ_PT, data, len);
    return true;
}

bool app_spp_cfu_send(uint8_t *data, uint16_t len)
{
    app_cmd_send_by_spp(SPP_CMD_CFU_PT, data, len);
    return true;
}

static void spp_eq_pt_received_proc(uint8_t *data, uint16_t len)
{
    if ((!data) || (!len))
    {
        return;
    }
    APP_PRINT_INFO1("spp_eq_pt_received_proc len 0x%x", len);
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    app_usb_hid_send_passthrough(data, len);
#endif
}

static void spp_cfu_pt_received_proc(uint8_t *data, uint16_t len)
{
    if ((!data) || (!len))
    {
        return;
    }
    APP_PRINT_INFO1("spp_cfu_pt_received_proc len 0x%x", len);
#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
    app_cfu_pt_handle_spp_received(data, len);
#endif
}

void app_spp_cmd_received(uint8_t *addr, uint8_t *data, uint16_t len)
{
    if ((!data) || (!len))
    {
        return;
    }

#ifdef LEGACY_BT_GENERAL
    APP_PRINT_INFO2("app_spp_cmd_received data %b len 0x%x", TRACE_BINARY(4, data), len);

    spp_cmd_t *p_spp = (spp_cmd_t *)data;

    /* only process cmd type */
    if (p_spp->type != 0x01)
    {
        return;
    }
    uint16_t pl_len = p_spp->pl + (p_spp->pl_high << 8);

    switch (p_spp->cmd)
    {
    case SPP_CMD_EQ_PT:
        spp_eq_pt_received_proc(data + SRC_SPP_HEADER_SIZE, pl_len);
        break;
    case SPP_CMD_CFU_PT:
        spp_cfu_pt_received_proc(data + SRC_SPP_HEADER_SIZE, pl_len);
        break;
    default:
        break;
    }
#endif
#ifdef LEGACY_BT_GAMING
    uint8_t spp_cmd = data[0];
    uint16_t spp_len = (len << 8) | (len >> 8 & 0x00ff) - 1;

    APP_PRINT_INFO3("app_spp_cmd_received data %b spp_cmd %x spp_len 0x%x", TRACE_BINARY(4, data + 1),
                    spp_cmd, spp_len);

    switch (spp_cmd)
    {
    case SPP_CMD_EQ_PT:
        spp_eq_pt_received_proc(data + 1, spp_len);
        break;

    case SPP_CMD_CFU_PT:
        spp_cfu_pt_received_proc(data + 1, spp_len);
        break;

#if F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT
    case DONGLE_CMD_ERASE_FLASH:
        {
            //Handle erase flash rsp here.

            if (data[1] == 0)
            {
                //Erase flash success
            }
            else
            {
                //Erase flash fail. Check data[1] for reason.
            }
        }
        break;

    case DONGLE_CMD_WRITE_CUSTOMIZED_VP:
        {
            //Handle write customized vp rsp here.

            if (data[1] == 0)
            {
                //write customized vp success.
            }
            else
            {
                /*
                    Write customized vp fail. Check data[1] for reason.
                    Notion: Since write flash fail,
                            it is needed to do erase flash and do write from first pkt.
                */
            }
        }
        break;
#endif

    case DONGLE_CMD_SYNC_STATUS:
        {
            memcpy(&headset_status, data + 1, sizeof(headset_status));
            app_gaming_handle_headset_status(&headset_status);
        }
        break;

#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
    case DONGLE_CMD_SPK_VOL_CTRL:
        {
            if (dongle_status.volume_sync_to_headset)
            {
                app_usb_audio_volume_hid_ctrl((T_APP_VOL_CTRL)data[1]);
            }
        }
        break;
#endif

    case DONGLE_CMD_MIC_MUTE_CTRL:
        {
            bool mic_mute = data[1];

            APP_PRINT_TRACE1("DONGLE_CMD_MIC_MUTE_CTRL: %d", mic_mute);

            app_usb_hid_send_telephony_mute_ctrl(mic_mute);
        }
        break;

    default:
        break;
    }
#endif
}

#if F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT
/*
Purpose:
    Send customized encoded VP data to headset.

Para:
    addr: flash start address.
    data: customized vp data.
    len:  customized vp data len.

Usage example:
    uint32_t addr = 0x238A000;
    data = get_data_from_usb;
    data_len = get_data_len_from_usb;

    app_spp_send_customized_vp_data(addr, data, data_len);
*/
void app_spp_send_customized_vp_data(uint32_t addr, uint8_t *data, uint16_t len)
{
    uint16_t total_len = len + 4;
    uint8_t *p_data = os_mem_zalloc(RAM_TYPE_DATA_ON, total_len);

    if (!p_data)
    {
        APP_PRINT_ERROR0("app_spp_send_customized_vp_data: zalloc fail");
        return;
    }

    /*Data Format: flash start addr + customized vp data */
    memcpy(p_data, &addr, 4);
    memcpy(p_data + 4, data, len);

    app_cmd_send_by_spp(DONGLE_CMD_WRITE_CUSTOMIZED_VP, p_data, total_len);

    os_mem_free(p_data);
}

/*
Purpose:
    Send customized encoded VP data to headset.

Para:
    None

Usage example:
    app_spp_send_write_vp_finish();
*/
void app_spp_send_write_vp_finish(void)
{
    app_cmd_send_by_spp(DONGLE_CMD_WRITE_VP_FINISH, NULL, 0);
}

/*
Purpose:
    Send cmd to erase specific area (4K) in headset.

Para:
    addr: start flash address wanna erase.

Usage example:
    uint32_t addr = 0x238A000;

    app_spp_erase_flash((uint8_t*)&addr);
*/
void app_spp_erase_flash(uint8_t *addr)
{
    app_cmd_send_by_spp(DONGLE_CMD_ERASE_FLASH, addr, 4);
}
#endif

#if (USB_PASSTHROUGH_CMD_SUPPORT==1)
void app_spp_register_usb_pt_cb(void)
{
    usb_pt_register_cb(app_spp_eq_send);
}
#endif

void app_spp_cmd_init(void)
{
#ifdef LEGACY_BT_GAMING
    spp_cmd_register_recv_cback(app_spp_cmd_received);
#endif
}
