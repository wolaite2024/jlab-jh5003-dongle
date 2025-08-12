#ifndef _APP_USB_LAYER_H_
#define _APP_USB_LAYER_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ual_bluetooth.h"
#include "ual_dev_mgr.h"
#include "le_audio_service.h"
#include "le_csis_client_service.h"
#include "app_msg.h"
#include "aics_client.h"

#define IO_MSG_TYPE_APP_USB_LAYER       0xF5

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define STREAM_TO_UINT32(u32, p) {u32 = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); (p) += 4;}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define STREAM_TO_UINT8(u8, p)   {u8 = (uint8_t)(*(p)); (p) += 1;}

#define UNBOND_TYPE                       0x00
#define BONDED_TYPE                       0x01

#define BOND_STATE_NONE                   0x00
#define BOND_STATE_BONDING                0x01
#define BOND_STATE_BONDED                 0x02

#define CONNECT_STATE                     0x01
#define SYNC_STATE                        0x02
#define CONFIGURED_STATE                  0x04
#define STREAM_STATE                      0x08
#define RELEASING_STATE                   0x10


#define BSK_SYNC_SRC_IDLE                 0x00
#define BSK_SYNC_PA_SYNCHRONIZING         0x01
#define BSK_SYNC_PA_SYNCHRONIZED          0x02
#define BSK_SYNC_BIG_SYNCHRONIZING        0x04
#define BSK_SYNC_BIG_SYNCHRONIZED         0x08

#define RTK_START_OPCODE                  0x5d00
//cmd opcode
#define GET_DEVICES_INFO_OPCODE           0x5d01
#define SCAN_START_OPCODE                 0x5d02
#define SCAN_STOP_OPCODE                  0x5d03
#define CONNECT_DEVICE_OPCODE             0x5d04
#define DISCONNECT_DEVICE_OPCODE          0x5d05
#define CREATE_BOND_OPCODE                0x5d06
#define REMOVE_BOND_OPCODE                0x5d07

#define SET_BROADCAST_TK_CODEC_OPCODE     0x5d10
#define START_BROADCAST_OPCODE            0x5d11
#define ADD_DEV_TO_BSGRP_OPCODE           0x5d12
#define RMV_DEV_FROM_BSGRP_OPCODE         0x5d13
#define ENABLE_BST_SYNC_OPCODE            0x5d14

#define SET_DEVICE_CONF_OPCODE            0x5d20
#define RMV_DEVICES_CONF_OPCODE           0x5d21
#define TRANSFER_OPERATION_OPCODE         0x5d22

#define BROADCAST_ASSIS_DISC_OPCODE       0x5d30
#define BC_AS_SELEC_BC_SRC_OPCODE         0x5d31
#define BC_AS_SYNC_BC_SRC_OPCODE          0x5d32
#define BC_AS_SET_REMOTE_SYNC_OPCODE      0x5d33
#define BC_AS_STOP_REMOTE_SYNC_OPCODE     0x5d34
#define BC_AS_MODIFY_REMOTE_SYNC_OPCODE   0x5d35
#define BC_SNK_SELEC_BC_SRC_OPCODE        0x5d36
#define BC_SNK_SYNC_BC_SRC_OPCODE         0x5d37

#define SET_VOLUME_OFFSET_OPCODE          0x5d40
#define SET_REMOTE_VOL_VAL_OPCODE         0x5d41
#define SET_REMOTE_MUTE_OPCODE            0x5d42
#define SET_MIC_MUTE_STATE_OPCODE         0x5d43
#define SET_AUDIO_INPUT_GAIN_VALUE_OPCODE 0x5d44
#define SET_AUDIO_INPUT_MUTE_OPCODE       0x5d45
#define SET_AUDIO_INPUT_GAIN_MODE_OPCODE  0x5d46
#define UAC_VOL_CHANGE_OPCODE             0x5d47

#define ENABLE_DISCOVER_SET_MEMBERS       0x5d50
#define CONNECT_ALL_SET_MEMBERS           0x5d51
#define SET_COORDINATORS_VOL_VAL_OPCODE   0x5d52
#define SET_COORDINATORS_MUTE_OPCODE      0x5d53

#define MODIFY_LE_WHITE_LIST              0x5DF1



#define COMMAND_COMPLETE_SUCCESS          0x00
#define UNKNOWN_COMMAND                   0x01
#define INVALID_COMMAND_LENGTH            0x02
#define INVALID_COMMAND_PARAM             0x03
#define COMMAND_OPERATION_FAIL            0x04

//event opcode
#define SCAN_STATE_CHANGE_EVENT           0x0E00
#define SCAN_RESULT_INFO_EVENT            0x0E01
#define BOND_STATE_CHANGE_EVENT           0x0E02
#define DEVICE_STATE_CHANGE_EVENT         0x0E03
#define DEVICE_PROPS_UPDATE_EVENT         0x0E04
#define DEVICE_CONFIG_INFO_EVENT          0x0E05
#define COORDINATE_SET_REPORT_EVENT       0x0E06
#define BROADCAST_ASSIST_SYNC_STATE_EVENT 0x0E09
#define BROADCAST_SRC_INFO_REPORT_EVENT   0x0E0A
#define AUDIO_INPUT_INFO_REPORT_EVENT     0x0E0B
#define VOLUME_SETTING_REPORT_EVENT       0x0E0C
#define VOLUME_OFFSET_REPORT_EVENT        0x0E0D
#define MIC_MUTE_STATE_REPORT_EVENT       0x0E10
#define DONGLE_PLAY_STATE_EVENT           0x0E0E
#define BST_RECV_STATE_REPORT_EVENT       0x0E0F

//internal event
#define AUDIO_PLAY_STATE_EVENT            0x0EFE
#define COMMAND_COMPLETE_EVENT            0x0EFF


#define SUPPORT_AUDIO_CHANNEL_ALOC_PROP   0x01
#define CURRENT_SAMPLING_FREQUENCY_PROP   0x02
#define CURRENT_AUDIO_CHANNEL_ALOC_PROP   0x03
#define AUDIO_SUPPORTED_CONTEXTS_PROP     0x04
#define AUDIO_AVAILABLE_CONTEXTS_PROP     0x05
#define AUDIO_ASE_ID_CONFIG_PROP          0x06
#define AUDIO_PAC_RECORD_PROP             0x07
#define AUDIO_VOLUME_OFFSET_VALUE_PROP    0x08
#define COORDINATOR_SET_DEV_ADDED_PROP    0x09
#define BROADCAST_SRC_INFO_PROP           0x0A
#define AUDIO_INPUT_INFO_PROP             0x0B

#define BROADCAST_MODE                    0x01
#define UNICAST_MEDIA_MODE                0x02
#define UNICAST_CONVERSATION_MODE         0x03

#define LE_AUDIO_IDLE_STATE               0x00
#define LE_AUDIO_STREAM_STARTING_STATE    0x01
#define LE_AUDIO_STREAMING_STATE          0x02
#define LE_AUDIO_RELEASING_STATE          0x03

#define BROADCAST_STOP                    0x00
#define BROADCAST_START                   0x01
#define BROADCAST_RELEASE                 0x02

#define SCAN_STOPPED                      0x00
#define SCAN_STARTED                      0x01

#define TRANSFER_STOP                     0x00
#define TRANSFER_MEDIA_START              0x01
#define TRANSFER_CONVERSATION_START       0x02
#define TRANSFER_SUSPEND                  0x03

#define CONNECT_MODE_DIRECT               0x00
#define CONNECT_MODE_ADD_WL               0x01

void bond_state_change(T_BT_BOND_INFO *p_bond_info);
void scan_state_changed(uint8_t state);
void ble_usb_audio_state_change(uint8_t *bd_addr, uint8_t bd_type, uint8_t conn_state,
                                uint8_t audio_state, uint16_t disc_cause);
void device_pac_info_update_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t role,
                                     uint16_t sup_freq, uint8_t chnl_cnts, uint16_t prefer_context,
                                     uint8_t metadata_len, uint8_t *p_metadata);
void device_cfg_report_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t *p_data, uint16_t len);

uint8_t usb_parse_host_cmd(uint8_t *p_data, uint16_t len);
void device_properities_update_callback(uint8_t *bd_addr, uint8_t bd_type,
                                        uint16_t snk_sup_context,
                                        uint16_t src_sup_context,
                                        uint16_t snk_avail_context, uint16_t src_avail_context,
                                        uint32_t snk_audio_loc, uint32_t src_audio_loc);
void scan_results_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t connect_mode,
                           uint8_t adv_sid, uint8_t *name, uint16_t name_len, int8_t rssi, uint8_t gaming_mode);
void volume_setting_report_callback(uint8_t *bd_addr, uint8_t volume_setting, uint8_t mute);
void volume_offset_val_report_callback(uint8_t *bd_addr, uint8_t idx,
                                       int16_t volume_offset, uint32_t audio_location);
void mic_mute_state_report_callback(uint8_t *bd_addr, uint8_t mute_state);

void ba_bst_src_info_callback(T_BROADCAST_SRC_INFO *p_bc_source);
void bst_src_sync_state_callback(uint8_t *bd_addr, uint8_t bd_type, uint8_t advertiser_sid,
                                 uint8_t sync_state);
void ba_brs_report_callback(uint8_t *bd_addr, uint8_t receiver_id,
                            T_BRS_INFO *p_brs_info);
void set_mem_avail_report_callback(T_SET_MEM_AVAIL *set_report);
void audio_input_info_report_callback(uint8_t *bd_addr, uint8_t instant_id,
                                      T_AICS_SRV_DATA *p_data);

void app_usb_layer_init(void);
void usb_layer_signal_handle(T_IO_MSG *msg);
bool scan_state_changed_signal(uint8_t state);
void le_audio_state_play_callback(uint8_t mode, uint8_t state);
bool le_audio_uac_vol_change(uint8_t vol, uint8_t mute);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */
#endif
