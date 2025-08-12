#include<stdbool.h>
#include<stdint.h>
#include <string.h>
#include "app_msg.h"

#define GIP_INT_OUT_MAX_TRANSMISSION_UNIT   0x40
#define GIP_INT_OUT_MAX_PENDING_REQ_NUM     0xa

typedef enum
{
    GIP_BT_EVT_CONN,
    GIP_BT_EVT_DISCONN,
} T_APP_GIP_BT_EVT;

static void app_gip_timer_callback(uint8_t timer_id, uint16_t timer_chann);

void app_gip_start(void);
void app_gip_init(void);
void app_gip_handle_bt_evt(T_APP_GIP_BT_EVT evt);
void app_gip_mode_switch_handle_msg(T_IO_MSG *msg);
void app_gip_mode_switch_init(void);
uint8_t app_gip_get_switch_mode(void);
void app_gip_handle_security_info(uint16_t pid, uint16_t vid, uint8_t *puid);
void app_gip_usb_msg_handle(T_IO_MSG *msg);

#if F_APP_GIP_CONTROLLER_WITH_AUDIO_SUPPORT
void app_gip_controller_audio_enable(bool enabled);
#endif
