#ifndef _APP_CTRL_PKT_POLICY_H_
#define _APP_CTRL_PKT_POLICY_H_

#include <stdint.h>
#include "app_gaming_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EARBUD_SIDE_ANY   = 0,
    EARBUD_SIDE_LEFT  = 1,
    EARBUD_SIDE_RIGHT = 2,
} T_EARBUD_SIDE;

bool app_cmd_send_by_le_fix_channel(T_APP_DONGLE_CMD cmd, uint8_t *data, uint16_t len,
                                    T_EARBUD_SIDE side);
void app_gaming_ctrl_data_rcv(uint8_t *data, uint16_t len);
bool app_gaming_ctrl_check_timer_need_to_restart(uint32_t curr_time, uint32_t last_start_time,
                                                 uint32_t detect_time);
bool app_gaming_ctrl_send_fix_channel_data(uint8_t *addr, uint16_t cid, uint8_t *p_data,
                                           uint16_t length, bool flushable);
bool app_gaming_ctrl_data_send_to_headset(uint8_t *data, uint16_t size, bool flushable);
void app_gaming_handle_headset_status(T_HEADSET_STATUS *headset_status);
void app_gaming_sync_dongle_status(void);
void app_gaming_ctrl_init(void);

#ifdef __cplusplus
}
#endif

#endif
