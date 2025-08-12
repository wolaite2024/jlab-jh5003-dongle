#ifndef _APP_LE_AUDIO_H_
#define _APP_LE_AUDIO_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include "gap.h"
#include "ual_bluetooth.h"

#if DONGLE_LE_AUDIO
void app_le_audio_init(void);
void app_le_audio_handle_msg(uint32_t param);
void app_le_audio_set_upstream_active(bool state);
void app_le_audio_set_downstream_active(bool state);
void app_le_audio_start_auto_pair(void);
void app_le_audio_enable_unicast_audio(bool enable);
void app_le_audio_parse_event(uint8_t *p_data, uint8_t len);
bool app_le_audio_is_in_idle(void);
void app_le_audio_handle_vol_notify(uint8_t *p_addr, uint8_t volume_setting, uint8_t mute);
void app_le_audio_set_remote_vol(uint8_t vol, uint8_t mute);
void app_le_audio_get_stream_state(void);
bool app_le_audio_set_pair_filter(uint8_t enable, uint8_t type, uint8_t *data, uint8_t data_len);
void app_le_audio_transmit_service_connected(void);
bool app_le_audio_scan_is_on(void);
void stop_le_audio_scan(void);
void start_le_audio_scan(void);
bool app_le_audio_restarting(void);
void app_le_audio_restart_stream(void);

#if F_APP_LEA_DONGLE_BINDING
bool app_le_audio_add_white_list(uint8_t *bd_addr, T_GAP_REMOTE_ADDR_TYPE bd_type);
void app_le_audio_clear_white_list(void);
void app_le_audio_clear_bud_record(void);
void app_le_audio_load_white_list(void);
bool app_le_audio_is_in_pairing(void);
void app_le_audio_cancel_pairing(void);
void app_le_audio_handle_conn_state_change(uint8_t *p_addr, uint8_t addr_type, uint8_t conn_state);
void app_le_audio_handle_auth_complete(uint8_t *p_addr);
void app_le_usb_cmd_handle_stop_scan(void);
void app_le_audio_direct_enable_unicast_audio(void);
#endif

#endif
#endif
