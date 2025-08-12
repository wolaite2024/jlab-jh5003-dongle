#ifndef _LE_SERVICE_MGR_H_
#define _LE_SERVICE_MGR_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <profile_client.h>
#include "os_queue.h"
#include "profile_server_def.h"
#include "profile_server.h"

#include "ual_bluetooth.h"


void le_service_gap_params_reset(void);
uint8_t le_service_get_comm_adv_data(uint8_t *p_data, uint8_t buff_len);
void ble_service_mgr_init(void *evt_queue, void *io_queue);

#endif
