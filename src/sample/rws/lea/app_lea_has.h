#ifndef _APP_LEA_HAS_H_
#define _APP_LEA_HAS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

typedef enum
{
    APP_IAS_TIMER_ALERT  = 0x00,
} T_APP_IAS_TIMER;

typedef enum
{
    APP_IAS_NO_ALERT = 0x00,
    APP_IAS_MILD_ALERT,
    APP_IAS_HIGH_ALERT
} T_APP_IAS_ALERT_LEVEL;

void app_lea_has_init(bool is_ha_cp_notify);
void app_lea_has_handle_authen_evt(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
