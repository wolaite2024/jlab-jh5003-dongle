#ifndef _LE_UNICAST_GAMING_H_
#define _LE_UNICAST_GAMING_H_

#include <stdio.h>
#include "le_unicast_src_service.h"

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#define GAMING_QOS_RTN                          (2)
#define GAMING_DURATION_7_5_MS_QOS_LATENCY      (0x08)
#define GAMING_DURATION_10_MS_QOS_LATENCY       (0x0A)
#define VENDOR_DATA_TYPE_GAMING_MODE           (0x01)

void le_gaming_cfg_qos(T_AUDIO_STREAM_SESSION_HANDLE handle, uint8_t prefer_idx);

bool le_gaming_handle_pac_info_update(uint8_t *bd_addr, uint8_t bd_type, uint8_t role,
                                      uint16_t sup_freq, uint8_t chnl_cnts, uint16_t prefer_context,
                                      uint8_t metadata_len, uint8_t *p_metadata);
bool le_unicast_is_gaming_mode(void);
void lea_gaming_init(void);

bool le_unicast_is_gaming_mode(void);

void app_switch_gaming_mode(uint8_t is_gaming_mode);

void le_unicast_set_gaming_mode(bool gaming);

void lea_gaming_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
