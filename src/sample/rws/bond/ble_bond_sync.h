#ifndef BLE_BOND_SYNC_H
#define BLE_BOND_SYNC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "gap_msg.h"
#include "app_link_util.h"

#if F_APP_BOND_MGR_BLE_SYNC
void ble_bond_sync_handle_remote_conn_cmpl(void);
void ble_bond_sync_handle_ble_mic_failure_disconn(T_APP_LE_LINK *p_link);
void ble_bond_sync_handle_disconn_cmpl(void);

void ble_bond_sync_init(void);


#endif

#ifdef __cplusplus
}
#endif

#endif /* BLE_AUDIO_BOND_H */
