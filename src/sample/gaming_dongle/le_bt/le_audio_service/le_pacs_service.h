#ifndef _LE_PACS_SERVICE_H_
#define _LE_PACS_SERVICE_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

#if LE_AUDIO_PACS_SUPPORT

extern uint16_t dongle_sink_available_contexts;
extern uint16_t dongle_source_available_contexts;

void le_pacs_init(void);
uint16_t le_pacs_handle_msg(T_LE_AUDIO_MSG msg, void *buf);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
