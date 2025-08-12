/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <string.h>

#include "bt_types.h"
#include "trace.h"
#include "os_mem.h"
#include "app_mmi.h"
#include "app_msg.h"
#include "app_console_msg.h"
#include "app_link_util.h"
#include "bt_hfp.h"
#include "bt_avrcp.h"
#include "btm.h"
#include "app_cli_main.h"
#include "gaming_bt.h"
#include "legacy_audio_wrapper.h"
#include "app_usb_layer.h"
#include "app_ctrl_cfg.h"
#include "app_general_policy.h"
#include "app_src_storage_target_dev.h"

#if UAL_CONSOLE_PRINT
#include "app_adapter_service.h"
#include "cli_console_cmd.h"
#include "ble_audio_def.h"
#include "codec_def.h"
#include "ccp_mgr.h"
#include "le_audio_data.h"
#include "le_ba_service.h"
#include "le_unicast_sink_service.h"
//#include "csis_sir.h"
#include "console.h"
#include "le_bass_service.h"
#include "teams_call_control.h"
#include "le_media_controller.h"

#if DONGLE_LE_AUDIO
#include "app_le_audio.h"
#endif

extern void audio_pipe_debug(void);
extern void (*DSP_LogOutput)(void);
extern bool dsp_ipc_set_tone_gain(uint16_t left_gain, uint16_t right_gain);
__weak void audio_pipe_debug(void)
{
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
    dsp_ipc_set_tone_gain(0, true);
}

#endif


#include "app_usb_hid.h"

void app_console_handle_msg(T_IO_MSG console_msg)
{
    uint16_t  subtype;
    uint16_t  id;
    uint8_t   action;
    uint8_t  *p;

    p       = console_msg.u.buf;
    subtype = console_msg.subtype;
    switch (subtype)
    {
    case IO_MSG_CONSOLE_STRING_RX:
        LE_STREAM_TO_UINT16(id, p);
#if UAL_CONSOLE_PRINT
        if (id & RTK_START_OPCODE)
        {
            APP_PRINT_TRACE1("app_console_handle_msg: RTK_START_OPCODE id 0x%04x", id);
            switch (id)
            {
            case GET_DEVICES_INFO_OPCODE:
                {
                    usb_parse_host_cmd(console_msg.u.buf, 2);
                }
                break;
            case SCAN_START_OPCODE:
                {
                    usb_parse_host_cmd(console_msg.u.buf, 4);
                }
                break;

            case SCAN_STOP_OPCODE:
                usb_parse_host_cmd(console_msg.u.buf, 2);
                break;

            case CREATE_BOND_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);

                    if (dev != NULL)
                    {
                        uint8_t data[10];
                        uint8_t *pp = data;
                        UINT16_TO_STREAM(pp, CREATE_BOND_OPCODE);
                        UINT8_TO_STREAM(pp, BONDED_TYPE);
                        UINT8_TO_STREAM(pp, dev->bd_type);
                        memcpy(pp, dev->bd_addr, BD_ADDR_LEN);
                        pp += BD_ADDR_LEN;
                        usb_parse_host_cmd(data, sizeof(data));
                    }
                    else
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg can not find dev %d", dev_idx);
                    }
                }
                break;

            case REMOVE_BOND_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev != NULL)
                    {
                        uint8_t data[9];
                        uint8_t *pp = data;
                        UINT16_TO_STREAM(pp, REMOVE_BOND_OPCODE);
                        UINT8_TO_STREAM(pp, dev->bd_type);
                        memcpy(pp, dev->bd_addr, GAP_BD_ADDR_LEN);
                        pp += GAP_BD_ADDR_LEN;
                        usb_parse_host_cmd(data, sizeof(data));
                    }
                }
                break;
            case SET_BROADCAST_TK_CODEC_OPCODE:
                {
                    uint8_t data[4];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_BROADCAST_TK_CODEC_OPCODE);
                    data[2] = *p;
                    data[3] = *(p + 1);
                    usb_parse_host_cmd(data, sizeof(data));
                }
                break;
            case START_BROADCAST_OPCODE:
                {
                    uint8_t data[19];
                    memset(data, 0, 19);
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, START_BROADCAST_OPCODE);
                    data[2] = *p;
                    usb_parse_host_cmd(data, 19);
                }
                break;

            case ADD_DEV_TO_BSGRP_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        return;
                    }
                    uint8_t data[9];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, ADD_DEV_TO_BSGRP_OPCODE);
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, GAP_BD_ADDR_LEN);
                    usb_parse_host_cmd(data, 9);
                }
                break;

            case RMV_DEV_FROM_BSGRP_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        return;
                    }
                    uint8_t data[9];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, RMV_DEV_FROM_BSGRP_OPCODE);
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, GAP_BD_ADDR_LEN);
                    usb_parse_host_cmd(data, 9);
                }

                break;

            case SET_DEVICE_CONF_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        return;
                    }
                    uint8_t data[33];
                    uint32_t chnl_loc = (uint32_t)(*p);
                    p++;
                    data[0] = SET_DEVICE_CONF_OPCODE & 0xff;
                    data[1] = (SET_DEVICE_CONF_OPCODE >> 8) & 0xff;
                    if (*p == BS_SET_CONFIGURE)
                    {
                        data[2] = BROADCAST_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, BD_ADDR_LEN);
                        data[10] = 6;
                        data[11] = 5;
                        data[12] = CURRENT_AUDIO_CHANNEL_ALOC_PROP;
                        memcpy(&data[13], &chnl_loc, 4);
                        usb_parse_host_cmd(data, 17);
                    }
                    else if (*p == UC_SET_CONFIGURE)
                    {
                        data[2] = UNICAST_MEDIA_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, BD_ADDR_LEN);
                        data[10] = 11;
                        data[11] = 10;
                        data[12] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[13] = 1; //ase id
                        data[14] = 0;// ignore ase state
                        data[15] = SERVER_AUDIO_SINK; //direction
                        data[16] = SAMPLING_FREQUENCY_CFG_48K;
                        memcpy(&data[17], &chnl_loc, 4);
                        data[21] = 0xFF;
                        usb_parse_host_cmd(data, 22);
                    }
                    else if (*p == UC_SET_CONFIGURE2)
                    {
                        data[2] = UNICAST_MEDIA_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, BD_ADDR_LEN);
                        data[10] = 11;
                        data[11] = 10;
                        data[12] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[13] = 2; //ase id
                        data[14] = 0;// ignore ase state
                        data[15] = SERVER_AUDIO_SINK; //direction
                        data[16] = SAMPLING_FREQUENCY_CFG_48K;
                        memcpy(&data[17], &chnl_loc, 4);
                        data[21] = 0xFF;
                        usb_parse_host_cmd(data, 22);
                    }
                    else if (*p == UC_SET_CONFIGURE3)
                    {
                        data[2] = UNICAST_MEDIA_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, BD_ADDR_LEN);
                        data[10] = 11;
                        data[11] = 10;
                        data[12] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[13] = 3; //ase id
                        data[14] = 0;// ignore ase state
                        data[15] = SERVER_AUDIO_SOURCE; //src direction
                        data[16] = SAMPLING_FREQUENCY_CFG_16K;
                        memcpy(&data[17], &chnl_loc, 4);
                        data[21] = 0xFF;
                        usb_parse_host_cmd(data, 22);
                    }
                    else if (*p == UC_SET_CONFIGURE4)
                    {
                        data[2] = UNICAST_MEDIA_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, BD_ADDR_LEN);
                        data[10] = 11;
                        data[11] = 10;
                        data[12] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[13] = 1; //ase id
                        data[14] = 0;// ignore ase state
                        data[15] = SERVER_AUDIO_SINK; //direction
                        data[16] = SAMPLING_FREQUENCY_CFG_16K;
                        memcpy(&data[17], &chnl_loc, 4);
                        data[21] = 0xFF;
                        usb_parse_host_cmd(data, 22);
                    }
                    else if (*p == UC_SET_CONFIGURE5)
                    {
                        chnl_loc = 1;
                        data[2] = UNICAST_CONVERSATION_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, BD_ADDR_LEN);
                        data[10] = 11;
                        data[11] = 10;
                        data[12] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[13] = 1; //ase id
                        data[14] = 0;// ignore ase state
                        data[15] = SERVER_AUDIO_SINK; //direction
                        data[16] = SAMPLING_FREQUENCY_CFG_16K;
                        memcpy(&data[17], &chnl_loc, 4);
                        data[21] = 0xFF;
                        usb_parse_host_cmd(data, 22);

                    }
                    else if (*p == UC_SET_CONFIGURE6)
                    {
                        chnl_loc = 1;
                        data[2] = UNICAST_CONVERSATION_MODE;
                        data[3] = dev->bd_type;
                        memcpy(&data[4], dev->bd_addr, GAP_BD_ADDR_LEN);
                        data[10] = 22;
                        data[11] = 10;
                        data[12] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[13] = 1; //ase id
                        data[14] = 0;// ignore ase state
                        data[15] = SERVER_AUDIO_SINK; //direction
                        data[16] = SAMPLING_FREQUENCY_CFG_16K;
                        memcpy(&data[17], &chnl_loc, 4);
                        data[21] = 0xFF;

                        chnl_loc = 1;

                        data[22] = 10;
                        data[23] = AUDIO_ASE_ID_CONFIG_PROP;
                        data[24] = 2; //ase id
                        data[25] = 0;// ignore ase state
                        data[26] = SERVER_AUDIO_SOURCE; //direction
                        data[27] = SAMPLING_FREQUENCY_CFG_16K;
                        memcpy(&data[28], &chnl_loc, 4);
                        data[32] = 0xFF;
                        usb_parse_host_cmd(data, 33);

                    }
                }
                break;

            case ENABLE_BST_SYNC_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        return;
                    }
                    uint8_t data[10];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, ENABLE_BST_SYNC_OPCODE);
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, BD_ADDR_LEN);
                    data[9] = *p;
                    usb_parse_host_cmd(data, 10);
                }
                break;

            case TRANSFER_OPERATION_OPCODE:
                {
                    uint8_t data[4];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, TRANSFER_OPERATION_OPCODE);
                    data[2] = *p++;
                    data[3] = *p;

                    usb_parse_host_cmd(data, 4);
                }
                break;

            case CONNECT_DEVICE_OPCODE:
                {
                    uint8_t dev_idx;
                    uint8_t mode;
                    LE_STREAM_TO_UINT8(mode, p);
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        return;
                    }
                    uint8_t data[10];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, CONNECT_DEVICE_OPCODE);
                    data[2] = mode;
                    data[3] = dev->bd_type;
                    memcpy(&data[4], dev->bd_addr, GAP_BD_ADDR_LEN);
                    usb_parse_host_cmd(data, 10);
                }
                break;

            case DISCONNECT_DEVICE_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        return;
                    }
                    uint8_t data[10];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, DISCONNECT_DEVICE_OPCODE);
                    data[2] = *p;
                    data[3] = dev->bd_type;
                    memcpy(&data[4], dev->bd_addr, GAP_BD_ADDR_LEN);
                    usb_parse_host_cmd(data, 10);
                }
                break;

            case SET_VOLUME_OFFSET_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    uint8_t data[12];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_VOLUME_OFFSET_OPCODE);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg SET_VOLUME_OFFSET_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, GAP_BD_ADDR_LEN);
                    LE_STREAM_TO_UINT8(data[9], p);
                    LE_STREAM_TO_UINT16(data[10], p);
                    usb_parse_host_cmd(data, 12);
                }
                break;

            case ENABLE_DISCOVER_SET_MEMBERS:
                {
                    uint8_t data[6] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, ENABLE_DISCOVER_SET_MEMBERS);
                    data[2] = p[0];
                    data[3] = p[1];
                    data[4] = p[2];
                    data[5] = p[3];
                    usb_parse_host_cmd(data, 6);
                }
                break;

            case CONNECT_ALL_SET_MEMBERS:
                {
                    uint8_t data[5] = {CONNECT_ALL_SET_MEMBERS & 0xFF, (CONNECT_ALL_SET_MEMBERS >> 8) & 0xFF, p[0], p[1], p[2]};
                    usb_parse_host_cmd(data, 5);
                }
                break;
            case BROADCAST_ASSIS_DISC_OPCODE:
                {
                    uint8_t enable = 0;
                    enable = p[0];
                    uint8_t data[3];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, BROADCAST_ASSIS_DISC_OPCODE);
                    data[2] = enable;
                    usb_parse_host_cmd(data, 3);
                }
                break;

            case BC_AS_SYNC_BC_SRC_OPCODE:
                {
                    uint8_t dev_idx;
                    uint8_t enable;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    LE_STREAM_TO_UINT8(enable, p);
                    T_BC_SRC_INFO *dev = ba_bsnk_find_bst_dev_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg BC_AS_SYNC_BC_SRC_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    uint8_t data[14];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, BC_AS_SYNC_BC_SRC_OPCODE);
                    data[2] = dev->advertiser_sid;
                    data[3] = dev->broadcast_id[0];
                    data[4] = dev->broadcast_id[1];
                    data[5] = dev->broadcast_id[2];
                    data[6] = dev->adv_addr_type;
                    memcpy(&data[7], dev->adv_addr, BD_ADDR_LEN);
                    data[13] = enable;
                    usb_parse_host_cmd(data, 14);
                }
                break;

            case BC_AS_SELEC_BC_SRC_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_BC_SRC_INFO *dev = ba_bsnk_find_bst_dev_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg BC_AS_SELEC_BC_SRC_OPCODE idx %d wrong", dev_idx);
                        return;
                    }
                    uint8_t data[29] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, BC_AS_SELEC_BC_SRC_OPCODE);
                    data[2] = dev->advertiser_sid;
                    data[3] = dev->broadcast_id[0];
                    data[4] = dev->broadcast_id[1];
                    data[5] = dev->broadcast_id[2];
                    data[6] = dev->adv_addr_type;
                    memcpy(&data[7], dev->adv_addr, BD_ADDR_LEN);
                    usb_parse_host_cmd(data, 29);
                }
                break;
            case BC_AS_SET_REMOTE_SYNC_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg BC_AS_SET_REMOTE_SYNC_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    uint8_t data[13] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, BC_AS_SET_REMOTE_SYNC_OPCODE);
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, GAP_BD_ADDR_LEN);
                    memcpy(&data[9], p, 4);
                    usb_parse_host_cmd(data, 13);
                }
                break;
            case SET_REMOTE_VOL_VAL_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg SET_REMOTE_VOL_VAL_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    uint8_t data[10] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_REMOTE_VOL_VAL_OPCODE);
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, BD_ADDR_LEN);
                    data[9] = *p;
                    usb_parse_host_cmd(data, 10);
                }
                break;
            case SET_REMOTE_MUTE_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg SET_REMOTE_MUTE_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    uint8_t data[10] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_REMOTE_MUTE_OPCODE);
                    data[2] = dev->bd_type;
                    memcpy(&data[3], dev->bd_addr, BD_ADDR_LEN);
                    data[9] = *p;
                    usb_parse_host_cmd(data, 10);
                }
                break;
            case SET_COORDINATORS_VOL_VAL_OPCODE:
                {
                    uint8_t data[3] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_COORDINATORS_VOL_VAL_OPCODE);
                    data[2] = *p;
                    usb_parse_host_cmd(data, 3);
                }
                break;
            case SET_COORDINATORS_MUTE_OPCODE:
                {
                    uint8_t data[3] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_COORDINATORS_MUTE_OPCODE);
                    data[2] = *p;
                    usb_parse_host_cmd(data, 3);
                }
                break;
            case BC_SNK_SELEC_BC_SRC_OPCODE:
                {
                    uint8_t dev_idx;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    T_BC_SRC_INFO *dev = ba_bsnk_find_bst_dev_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg BC_SNK_SELEC_BC_SRC_OPCODE idx %d wrong", dev_idx);
                        return;
                    }
                    uint8_t data[29] = {0};
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, BC_SNK_SELEC_BC_SRC_OPCODE);
                    data[2] = dev->advertiser_sid;
                    data[3] = dev->broadcast_id[0];
                    data[4] = dev->broadcast_id[1];
                    data[5] = dev->broadcast_id[2];
                    data[6] = dev->adv_addr_type;
                    memcpy(&data[7], dev->adv_addr, BD_ADDR_LEN);
                    usb_parse_host_cmd(data, 29);
                }
                break;

            case BC_SNK_SYNC_BC_SRC_OPCODE:
                {
                    uint8_t dev_idx;
                    uint8_t bis;
                    uint8_t enable;
                    uint8_t num = 0;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    LE_STREAM_TO_UINT8(bis, p);
                    LE_STREAM_TO_UINT8(enable, p);
                    T_BC_SRC_INFO *dev = ba_bsnk_find_bst_dev_by_idx(dev_idx);
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg BC_SNK_SYNC_BC_SRC_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    APP_PRINT_ERROR2(" BC_SNK_SYNC_BC_SRC_OPCODE bis %d enable %d", bis, enable);
                    uint8_t data[28];
                    dev_idx = 0;
                    while (bis > 0)
                    {
                        dev_idx++;
                        if (bis & 0x01)
                        {
                            num++;
                            data[14 + num] = dev_idx;
                        }
                        bis = bis >> 1;
                    }

                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, BC_SNK_SYNC_BC_SRC_OPCODE);

                    data[2] = dev->advertiser_sid;
                    data[3] = dev->broadcast_id[0];
                    data[4] = dev->broadcast_id[1];
                    data[5] = dev->broadcast_id[2];
                    data[6] = dev->adv_addr_type;
                    memcpy(&data[7], dev->adv_addr, BD_ADDR_LEN);
                    data[13] = enable;
                    data[14] = num;
                    usb_parse_host_cmd(data, 14 + num);
                }
                break;
            case SET_MIC_MUTE_STATE_OPCODE:
                {
                    uint8_t dev_idx;
                    uint8_t enable;
                    LE_STREAM_TO_UINT8(dev_idx, p);
                    LE_STREAM_TO_UINT8(enable, p);
                    T_DEV_RES_INFO *dev = disc_mgr_find_device_by_idx(dev_idx);
                    uint8_t data[9];
                    if (dev == NULL)
                    {
                        APP_PRINT_ERROR1("app_console_handle_msg SET_MIC_MUTE_STATE_OPCODE idx %d wrong", dev_idx);
                        return ;
                    }
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, SET_MIC_MUTE_STATE_OPCODE);
                    memcpy(&data[2], dev->bd_addr, BD_ADDR_LEN);
                    data[8] = enable;
                    usb_parse_host_cmd(data, 9);
                }
                break;

            case 0x5dFE:
                {
                    uint8_t key;
                    LE_STREAM_TO_UINT8(key, p);
                    app_usb_hid_send_consumer_ctrl_key_down((enum CONSUMER_CTRL_KEY_CODE)key);
                    app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
                }
                break;

            case 0x5dFA:
                {
#if LE_AUDIO_ASCS_SUPPORT
                    uint8_t enable;
                    LE_STREAM_TO_UINT8(enable, p);
                    if (enable)
                    {
                        le_unicast_snk_start_adv();
                    }
                    else
                    {
                        le_unicast_snk_stop_adv();
                    }
#endif
                }
                break;
            case 0x5dF9:
                {
#if LE_AUDIO_BASS_SUPPORT
                    uint8_t enable;
                    LE_STREAM_TO_UINT8(enable, p);
                    if (enable)
                    {
                        le_bass_start_adv();
                    }
                    else
                    {
                        le_bass_stop_adv();
                    }
#endif
                }
                break;
            case 0x5dF8:
                {
                    audio_pipe_debug();
                }
                break;
            case 0x5dF7:
                {
                    uint8_t path;
                    LE_STREAM_TO_UINT8(path, p);
                    le_audio_set_audio_path(path);
                }
                break;
            case 0x5dF6:
                {
#if LE_AUDIO_MCP_CLIENT_SUPPORT
                    uint8_t conn_id;
                    uint8_t opcode;
                    LE_STREAM_TO_UINT8(conn_id, p);
                    LE_STREAM_TO_UINT8(opcode, p);
                    le_media_send_control_key(conn_id, opcode);
#endif
                }
                break;
            case 0x5dF5:
                {
#if TARGET_RTL8773DO
//                    DSP_LogOutput();
#else
                    DSP_LogOutput();
#endif
                }
                break;
            case 0x5dF4:
                {
#if DONGLE_LE_AUDIO
                    uint8_t enable;
                    LE_STREAM_TO_UINT8(enable, p);
                    app_le_audio_enable_unicast_audio(enable);
#endif
                }
                break;
            case 0x5dF3:
                {
#if DONGLE_LE_AUDIO
                    app_le_audio_start_auto_pair();
#endif
                }
                break;

            case 0x5dF2:
                {
#if DONGLE_LE_AUDIO
                    APP_PRINT_INFO1("string %b", TRACE_BINARY(20, p));
                    uint8_t enable;
                    uint8_t type = 0;
                    uint8_t len = 7;
                    LE_STREAM_TO_UINT8(enable, p);
                    LE_STREAM_TO_UINT8(type, p);
                    if (type) // name_str
                    {
                        len = strlen((char *)p);
                    }
                    app_le_audio_set_pair_filter(enable, type, p, len);
#endif
                }
                break;
            case MODIFY_LE_WHITE_LIST:
                {
                    uint8_t data[10];
                    uint8_t *pp = data;
                    UINT16_TO_STREAM(pp, MODIFY_LE_WHITE_LIST);
                    data[2] = p[0];
                    data[3] = p[1];
                    memcpy(data + 4, p + 2, 6);
                    usb_parse_host_cmd(data, 10);
                }
                break;
            }
        }
#endif

#ifdef LEGACY_BT_GAMING
        if ((id & 0xff00) == RTK_START_OPCODE)
        {
            switch (id)
            {
            case 0x5dc0:
                {
                }
                break;
            case GAMING_DISCOV_OPCODE:
                if (*p)
                {
                    gaming_bt_start_discovery();
                }
                else
                {
                    gaming_bt_stop_discovery();
                }
                break;
            case GAMING_CONN_OPCODE:
                gaming_bt_connect(*p, p + 1);
                break;
            case GAMING_DISC_BY_ID_OPCODE:
                gaming_bt_disconnect_by_id(*p);
                break;
            case GAMING_DISC_BY_ADDR_OPCODE:
                gaming_bt_disconnect_by_bdaddr(p);
                break;
            case GAMING_RM_BY_ID_OPCODE:
                gaming_bt_remove_bond_by_id(*p);
                break;
            case GAMING_RM_BY_ADDR_OPCODE:
                gaming_bt_remove_bond_by_bdaddr(p);
                break;
            }
        }
#endif
        if (id == APP_CMD)
        {
            LE_STREAM_TO_UINT8(action, p);
            APP_PRINT_INFO1("Get app cmd %d", action);
        }

#ifdef LEGACY_BT_GENERAL
        if ((id & 0xff00) == GENERAL_RTK_START_OPCODE)
        {
            switch (id)
            {
            case LEGACY_REMOVE_DEVICE_OPCODE:
                {
                    src_remove_audio_device();
                    app_src_storage_clear_target_dev(DEV_SLOT_HS);
                }
                break;
            case LEGACY_PAIR_TO_DEVICE_OPCODE:
                {
                    p++;
                    bool ret;
                    ret = src_connect_audio_device(p, 0);
                    APP_PRINT_INFO2("LEGACY_PAIR_TO_DEVICE_OPCODE, bd_addr: %b, src_connect_audio_device is %d",
                                    TRACE_BDADDR(p), ret);
                }
                break;
            case LEGACY_SET_BT_CONN_STATE_OPCODE:
                {
                    uint8_t status = LEGACY_COMMAND_COMPLETE_SUCCESS;
                    uint8_t conn_state;
                    uint8_t ret;
                    memcpy(&conn_state, p, 1);
                    if (conn_state > 1)
                    {
                        status = LEGACY_INVALID_COMMAND_PARAM;
                    }
                    else if (conn_state == 0x00)
                    {
                        ret = src_disconnect();
                        if (!ret)
                        {
                            status = LEGACY_COMMAND_OPERATION_FAIL;
                        }
                    }
                    else if (conn_state == 0x01)
                    {
                        ret = src_connect();
                        if (!ret)
                        {
                            status = LEGACY_COMMAND_OPERATION_FAIL;
                        }
                    }
                    APP_PRINT_INFO2("LEGACY_SET_BT_CONN_STATE_OPCODE, conn_state: %02x, status is %02x",
                                    conn_state, status);
                }
                break;
            }
        }
#endif

        os_mem_free(console_msg.u.buf);
        break;

    case  IO_MSG_CONSOLE_BINARY_RX:
//        p += 1;
//        LE_STREAM_TO_UINT8(rx_seqn, p);
//        LE_STREAM_TO_UINT16(cmd_len, p);
//        app_handle_cmd_set(p, cmd_len, CMD_PATH_UART, rx_seqn, 0);
        os_mem_free(console_msg.u.buf);
        break;

    default:
        break;

    }

}

