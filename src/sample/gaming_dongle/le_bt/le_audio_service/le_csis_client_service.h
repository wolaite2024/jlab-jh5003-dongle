#ifndef _LE_CSIS_CLIENT_SERVICE_H_
#define _LE_CSIS_CLIENT_SERVICE_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <gap_conn_le.h>
#include <profile_client.h>
#include "csis_rsi.h"
#include "ble_audio_def.h"
#include "vector.h"
#include "os_queue.h"
#include "csis_client.h"
#include "profile_server_def.h"
#include "ual_bluetooth.h"
#include "ble_audio_group.h"
#include "bass_client.h"
#include "ual_types.h"
#include "ual_list.h"
#include "gap.h"
#include "ble_audio.h"


typedef struct t_le_audio_csis
{
    T_BLE_AUDIO_GROUP_HANDLE group_handle;
    T_UALIST_HEAD           list;
    uint8_t                 hash[3];
    uint8_t                 size;
    uint16_t                srv_uuid;
} T_LE_AUDIO_CSIS;

typedef struct
{
    uint8_t                 bd_addr[6];
    uint8_t                 bd_type;
    uint16_t                srv_uuid;
    uint8_t                 rank;
    uint8_t                 size;
    uint8_t                 hash[3];
} T_SET_MEM_AVAIL;

void le_csis_client_gen_local_hash(uint8_t hash[3], uint8_t *p_sirk);
void le_csis_client_add_group_mem(T_CSIS_CLIENT_SET_MEM_FOUND *set_report);
bool le_csis_client_connect_group_all_mems(uint8_t hash[3]);
void le_csis_client_remove_group(T_BLE_AUDIO_GROUP_HANDLE group_handle);
void le_csis_client_remove_all_groups(void);
void le_csis_client_report_mem_info(uint8_t *bd_addr, uint8_t bd_type);

void le_csis_client_handle_bond_state_change(T_BT_BOND_INFO *p_bond_info);
void le_csis_client_handle_mem_rsi(uint8_t *rsi, uint8_t bd_type, uint8_t *bd_addr);
uint16_t le_csis_client_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

int le_csis_client_start_scan(uint8_t hash[3]);
void le_csis_client_stop_scan(void);

void le_csis_client_init(void);
#endif
