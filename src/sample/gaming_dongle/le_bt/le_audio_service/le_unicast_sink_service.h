#ifndef _LE_UNICAST_SINK_SERVICE_H_
#define _LE_UNICAST_SINK_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

#if LE_AUDIO_ASCS_SUPPORT

void le_unicast_sink_init(void);
uint16_t le_unicast_sink_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
void le_unicast_snk_start_adv(void);
void le_unicast_snk_stop_adv(void);
#endif

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */
#endif

