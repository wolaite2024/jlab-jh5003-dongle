#ifndef _APP_BLE_TIMER_H_
#define _APP_BLE_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdint.h"
/**
 * @brief app_ble_timer_init
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 */
void app_ble_timer_init(void);

/**
 * @brief set current link connection id for check
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 * @param conn_id
 */
void app_ble_timer_set_current_link_conn_id(uint8_t conn_id);

/**
 * @brief start timer, when timeout check if current ble link is common link
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 */
void app_ble_timer_start_check_common_link(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
