#ifndef _LE_MEDIA_PLAYER_H_
#define _LE_MEDIA_PLAYER_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "profile_server.h"
#include "ble_audio.h"
#if LE_AUDIO_MCP_SERVER_SUPPORT

typedef struct
{
    T_SERVER_ID   mcs_id;
    uint8_t       media_state;
    uint16_t      mcs_enabled_cccd;
    uint8_t       mcp_fast_opcode_timer;
} T_MEDIA_PLAYER_CB;

void le_media_player_init(T_SERVER_ID mcs_id);
T_SERVER_ID get_media_player_id(void);

void le_media_player_set_state(uint8_t state);

bool le_media_player_handle_operation(uint8_t opcode);
uint16_t le_media_player_handle_mcs_msg(T_LE_AUDIO_MSG msg, void *buf);

uint16_t le_mcs_get_enabled_cccd(void);

bool le_mcs_handle_server_msg(T_LE_AUDIO_MSG msg, void *buf);

#endif
#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
