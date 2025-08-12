#ifndef _LE_MEDIA_CONTROLLER_H_
#define _LE_MEDIA_CONTROLLER_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "le_audio_service.h"
#include "ble_audio.h"

#if LE_AUDIO_MCP_CLIENT_SUPPORT
uint16_t le_mcp_handle_msg(T_LE_AUDIO_MSG msg, void *buf);
void le_media_send_control_key(uint8_t conn_id, uint8_t opcode);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
