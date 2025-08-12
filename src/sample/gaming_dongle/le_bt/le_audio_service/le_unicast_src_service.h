#ifndef _LE_UNICAST_SRC_SERVICE_H_
#define _LE_UNICAST_SRC_SERVICE_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <gap_conn_le.h>
#include <profile_client.h>
#include "csis_rsi.h"
#include "csis_def.h"
#include "ble_audio_def.h"
#include "vector.h"
#include "os_queue.h"
#include "csis_client.h"
#include "profile_server_def.h"
#include "ual_bluetooth.h"
#include "ble_audio_group.h"
#include "ual_types.h"
#include "ual_list.h"
#include "bap.h"
#include "ascs_def.h"
#include "bap_unicast_client.h"

#define BDADDR_NONE { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }

typedef enum
{
    CONN_UPDATE_EVENT_LE_CONNECTED,         //0x00
    CONN_UPDATE_EVENT_LE_DISCONNECTED,      //0x01
    CONN_UPDATE_EVENT_UPDATE_MTU,           //0x02
    CONN_UPDATE_EVENT_AUTHEN_COMPLETE,      //0x03
    CONN_UPDATE_EVENT_MTU_UPDATE,           //0x04
    CONN_UPDATE_EVENT_GATT_DISCOVERY,       //0x05
    CONN_UPDATE_EVENT_DISCV_ALL_DONE,       //0x06
    CONN_UPDATE_EVENT_CREATE_CIS,           //0x07
    CONN_UPDATE_EVENT_CIS_DISCONNECT,       //0x08
    CONN_UPDATE_EVENT_HEADSET_BT_STREAMING, //0x09
    CONN_UPDATE_EVENT_HEADSET_BT_IDLE,      //0x0a
} T_LE_CONN_UPDATE_EVT;

typedef struct
{
    uint8_t           ase_id;
    uint8_t           real_ase_id;
    uint8_t           direction;
    uint8_t           sample_freq;
    uint32_t          chnl_loc;
    uint8_t           prefer_codec_index;
    uint8_t           ase_id_associated;
} T_UNICAST_ASE_PARAM;

typedef struct
{
    uint8_t           state;
    T_UNICAST_ASE_PARAM param;
} T_UNICAST_ASE_CB;

typedef struct t_unicast_dev_user_cfg
{
    T_UALIST_HEAD     list;
    uint8_t           bdaddr[6];
    uint8_t           mode;
    uint8_t           params_count;
    T_UNICAST_ASE_PARAM *params;
} T_UNICAST_DEV_USER_CFG;

#define UNICAST_PLAY_MODE_NONE                0x00
#define UNICAST_MEDIA_PLAY_MODE               0x01
#define UNICAST_CONVERSATION_PLAY_MODE        0x02

void le_audio_unicast_src_group_cb(T_AUDIO_GROUP_MSG msg,
                                   T_BLE_AUDIO_GROUP_HANDLE handle,
                                   void *buf);
void le_audio_unicast_handle_pacs_discover(T_BAP_DIS_ALL_DONE *p_data);
void le_audio_get_ase_info(T_BAP_DIS_ALL_DONE *p_data);
bool le_audio_add_unicast_cfg(uint8_t *bd_addr, uint8_t mode, uint8_t param_num,
                              T_UNICAST_ASE_PARAM *param);
bool le_audio_add_unicast_user_cfg(uint8_t *bdaddr, uint8_t mode,
                                   uint8_t num, T_UNICAST_ASE_PARAM *params);
bool le_audio_start_unicast_stream(uint8_t mode, uint8_t qos, bool gaming);
bool le_audio_user_start_unicast_stream(uint8_t mode, uint8_t qos, bool gaming);
bool le_unicast_stop_stream(void);
bool le_unicast_suspend_stream(void);
void le_unicast_handle_gap_msg(void *p_msg);
void le_unicast_src_service_init(T_SERVER_ID mcs_id, T_SERVER_ID ccp_id);
bool le_audio_clear_unicast_device_cfg(uint8_t num, uint8_t *p_addrs);
bool le_audio_clear_unicast_user_cfg(uint8_t num, uint8_t *p_addrs);
void le_audio_disconnect_unicast_device(uint8_t *bdaddr);
uint8_t le_unicast_src_play_info(uint8_t *p_qos_type, uint8_t *p_play_mode);
uint8_t le_audio_get_unicast_play_mode(void);
uint8_t le_unicast_src_conn_num(void);
void le_unicast_src_csis_disc_done(T_CSIS_CLIENT_DIS_DONE *dis_done);
void le_unicast_src_disconnected(uint8_t *bdaddr, uint8_t bdaddr_type);
bool le_audio_update_unicast_addr(uint8_t *pseudo_bdaddr, uint8_t bdaddr_type);
bool le_unicast_src_update_group_handle(T_CSIS_CLIENT_SET_MEM_FOUND *p_mem);
bool lea_gaming_update_upstream(bool enable);

void le_unicast_src_release_group(void);
void le_unicast_disable_conn_update(bool disable);
void le_unicast_conn_update_mgr(T_LE_CONN_UPDATE_EVT evt, uint8_t conn_id);

#endif
