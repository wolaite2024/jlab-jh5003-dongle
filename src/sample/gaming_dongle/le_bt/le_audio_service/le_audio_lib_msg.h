#ifndef _LE_AUDIO_LIB_MSG_H_
#define _LE_AUDIO_LIB_MSG_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "ble_audio.h"

uint16_t ble_audio_msg_cb(T_LE_AUDIO_MSG msg, void *buf);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
