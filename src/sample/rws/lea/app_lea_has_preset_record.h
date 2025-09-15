#ifndef _APP_LEA_HAS_PRESET_RECORD_H_
#define _APP_LEA_HAS_PRESET_RECORD_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */
#include "has_mgr.h"
#include "has_def.h"

typedef struct t_lea_has_preset_queue_elem
{
    struct t_lea_has_preset_queue_elem *p_next;
    uint8_t name_len;
    T_HAS_PRESET_FORMAT has_preset_record;
} T_LEA_HAS_PRESET_QUEUE_ELEM;

typedef enum
{
    PRESET_CHANGE_ADD = 0x01,
    PRESET_CHANGE_DELETE,
    PRESET_CHANGE_AVAILABLE,
    PRESET_CHANGE_UNAVAILABLE,
    PRESET_CHANGE_NAME_CHANGE
} T_LEA_HAS_CHANGE_PRESET_TYPE;

typedef enum
{
    HAS_MSG_SYNC_LOCALLY       = 0x01,
    HAS_MSG_SYNC_BY_AUDIO_CONN = 0x02,
    HAS_MSG_TOTAL
} T_LEA_HAS_REMOTE_MSG;

void app_lea_has_preset_record_init(void);
bool app_lea_has_modify_preset_record_list(T_LEA_HAS_CHANGE_PRESET_TYPE type, uint8_t idx,
                                           uint8_t preset_prop, uint8_t length,
                                           char *p_value, bool is_connect);
bool app_lea_has_handle_cccd_info(T_HAS_PENDING_PRESET_CHANGE *p_cccd_info);
uint16_t app_lea_has_handle_write_cp_req(T_HAS_CP_IND *p_write_req);
void app_lea_has_update_preset_idx_by_audio_conn(uint8_t id);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
