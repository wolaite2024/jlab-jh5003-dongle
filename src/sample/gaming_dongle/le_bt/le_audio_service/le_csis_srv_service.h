#ifndef _LE_CSIS_SRV_SERVICE_H_
#define _LE_CSIS_SRV_SERVICE_H_
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
#include "profile_server_def.h"
#include "ual_bluetooth.h"
#include "ble_audio_group.h"
#include "ual_types.h"
#include "ual_list.h"
#include "cap.h"

#if LE_AUDIO_CSIS_SUPPORT
uint16_t le_csis_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
bool le_csis_srv_get_rsi(uint8_t *p_rsi);
void le_csis_srv_init(T_CAP_INIT_PARAMS *p_param);
#endif
#endif
