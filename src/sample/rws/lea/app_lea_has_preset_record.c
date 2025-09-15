#include <stdlib.h>
#include "trace.h"
#include "os_mem.h"
#include "os_queue.h"
#include "ble_audio.h"
#include "app_cfg.h"
#include "app_hearable.h"
#include "app_lea_has_preset_record.h"
#include "app_main.h"
#include "app_relay.h"

#if F_APP_HAS_SUPPORT
typedef struct t_lea_has_preset_change_queue_elem
{
    struct t_lea_has_preset_change_queue_elem *p_next;
    uint8_t idx;
    T_HAS_PRESET_CHANGE_ID change_id;
} T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM;

T_OS_QUEUE lea_has_preset_change_list;
T_OS_QUEUE lea_has_preset_record_list;

T_LEA_HAS_PRESET_QUEUE_ELEM *app_lea_has_find_preset_record(uint8_t idx)
{
    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = NULL;

    for (uint8_t i = 0; i < lea_has_preset_record_list.count; i++)
    {
        p_preset_record = os_queue_peek(&lea_has_preset_record_list, i);
        if ((p_preset_record != NULL) && (p_preset_record->has_preset_record.index == idx))
        {
            APP_PRINT_INFO1("app_lea_has_find_preset_record: find preset index 0x%x",
                            p_preset_record->has_preset_record.index);
            return p_preset_record;
        }
    }

    return NULL;
}

uint8_t app_lea_has_get_prev_preset_idx(uint8_t cur_idx)
{
    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = NULL;

    for (uint8_t i = 0; i < lea_has_preset_record_list.count; i++)
    {
        p_preset_record = os_queue_peek(&lea_has_preset_record_list, i);
        if ((p_preset_record != NULL) && (p_preset_record->has_preset_record.index == cur_idx))
        {
            if (i == 0)
            {
                return 0;
            }
            else
            {
                p_preset_record = os_queue_peek(&lea_has_preset_record_list, i - 1);
                return p_preset_record->has_preset_record.index;
            }
        }
    }

    return 0;
}


T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM *app_lea_has_find_change_preset(uint8_t idx, uint8_t *change_id)
{
    T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM *p_change_idx = NULL;

    for (uint8_t i = 0; i < lea_has_preset_change_list.count; i++)
    {
        p_change_idx = os_queue_peek(&lea_has_preset_change_list, i);

        if ((p_change_idx != NULL) && (p_change_idx->idx == idx))
        {
            *change_id = p_change_idx->change_id;
            return p_change_idx;
        }
    }

    *change_id = 0xff;
    return NULL;
}

void app_lea_has_set_preset_change_idx_in_queue(T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM *p_change_idx)
{
    if (lea_has_preset_change_list.count == 0)
    {
        os_queue_in(&lea_has_preset_change_list, p_change_idx);
    }
    else
    {
        T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM *p_change_preset = NULL;

        for (uint8_t i = 0; i < lea_has_preset_change_list.count; i++)
        {
            p_change_preset = os_queue_peek(&lea_has_preset_change_list, i);

            if (p_change_preset->idx > p_change_idx->idx)
            {
                if (i == 0)
                {
                    os_queue_insert(&lea_has_preset_change_list, NULL, p_change_idx);
                    break;
                }
                else
                {
                    p_change_preset = os_queue_peek(&lea_has_preset_change_list, i - 1);
                    os_queue_insert(&lea_has_preset_change_list, p_change_preset, p_change_idx);
                    break;
                }
            }

            if ((i == lea_has_preset_change_list.count - 1) && (p_change_preset->idx < p_change_idx->idx))
            {
                os_queue_in(&lea_has_preset_change_list, p_change_idx);
                break;
            }
        }
    }
}

bool app_lea_has_save_preset_change_idx(T_HAS_PRESET_CHANGE_ID change_id, uint8_t index)
{
    uint8_t find_change_id = 0xff;
    T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM *p_change_idx = app_lea_has_find_change_preset(index,
                                                       &find_change_id);

    if (p_change_idx == NULL)
    {
        p_change_idx = malloc(sizeof(T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM));
        if (p_change_idx != NULL)
        {
            p_change_idx->idx = index;
            p_change_idx->change_id = change_id;
            app_lea_has_set_preset_change_idx_in_queue(p_change_idx);
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (find_change_id != change_id)
        {
            if (find_change_id == GENERIC_UPDATE)
            {
                if (change_id == PRESET_RECORD_DELETED)
                {
                    p_change_idx->change_id = PRESET_RECORD_DELETED;
                }
            }
            else if (find_change_id == PRESET_RECORD_DELETED)
            {
                if (change_id == GENERIC_UPDATE)
                {
                    p_change_idx->change_id = GENERIC_UPDATE;
                }
            }
            else if (find_change_id == PRESET_RECORD_AVAILABLE)
            {
                if (change_id == PRESET_RECORD_UNAVAILABLE)
                {
                    os_queue_delete(&lea_has_preset_change_list, p_change_idx);
                    free(p_change_idx);
                }
                else if (change_id == PRESET_RECORD_DELETED)
                {
                    p_change_idx->change_id = PRESET_RECORD_DELETED;
                }
                else
                {
                    p_change_idx->change_id = GENERIC_UPDATE;
                }
            }
            else if (find_change_id == PRESET_RECORD_UNAVAILABLE)
            {
                if (change_id == PRESET_RECORD_AVAILABLE)
                {
                    os_queue_delete(&lea_has_preset_change_list, p_change_idx);
                    free(p_change_idx);
                }
                else if (change_id == PRESET_RECORD_DELETED)
                {
                    p_change_idx->change_id = PRESET_RECORD_DELETED;
                }
                else
                {
                    p_change_idx->change_id = GENERIC_UPDATE;
                }
            }
        }
    }

    return true;
}

bool app_lea_has_modify_preset_record_list(T_LEA_HAS_CHANGE_PRESET_TYPE type, uint8_t idx,
                                           uint8_t preset_prop,
                                           uint8_t length, char *p_value, bool is_connect)
{
    switch (type)
    {
    case PRESET_CHANGE_ADD:
        {
            if (idx > HAS_MAX_PRESET_IDX)
            {
                APP_PRINT_ERROR1("app_lea_has_modify_preset_record_list: preset index too large, index %d", idx);
                return false;
            }

            if ((length < HAS_MIN_PRESET_NAME_LENGTH) && (length > HAS_MAX_PRESET_NAME_LENGTH))
            {
                APP_PRINT_ERROR1("app_lea_has_modify_preset_record_list: name too long, type %d", type);
                return false;
            }

            T_LEA_HAS_PRESET_QUEUE_ELEM *p_new_preset_record = malloc(sizeof(T_LEA_HAS_PRESET_QUEUE_ELEM));
            if (p_new_preset_record != NULL)
            {
                p_new_preset_record->has_preset_record.p_name = malloc(length);
                if (p_new_preset_record->has_preset_record.p_name == NULL)
                {
                    free(p_new_preset_record);
                    return  false;
                }

                p_new_preset_record->has_preset_record.index = idx;
                p_new_preset_record->has_preset_record.properties = preset_prop;
                memcpy(p_new_preset_record->has_preset_record.p_name, p_value, length);
                p_new_preset_record->name_len = length;

                if (lea_has_preset_record_list.count == 0)
                {
                    os_queue_in(&lea_has_preset_record_list, p_new_preset_record);
                }
                else
                {
                    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = NULL;

                    for (uint8_t i = 0; i < lea_has_preset_record_list.count; i++)
                    {
                        p_preset_record = os_queue_peek(&lea_has_preset_record_list, i);

                        if (p_preset_record->has_preset_record.index > p_new_preset_record->has_preset_record.index)
                        {
                            if (i == 0)
                            {
                                os_queue_insert(&lea_has_preset_record_list, NULL, p_new_preset_record);
                                break;
                            }
                            else
                            {
                                p_preset_record = os_queue_peek(&lea_has_preset_record_list, i - 1);
                                os_queue_insert(&lea_has_preset_record_list, p_preset_record, p_new_preset_record);
                                break;
                            }
                        }
                        else if (p_preset_record->has_preset_record.index == p_new_preset_record->has_preset_record.index)
                        {
                            APP_PRINT_ERROR1("app_lea_has_modify_preset_record_list: index already exist, type %d", type);
                            free(p_new_preset_record);
                            free(p_new_preset_record->has_preset_record.p_name);
                            return false;
                        }
                        else if ((p_preset_record->has_preset_record.index < p_new_preset_record->has_preset_record.index)
                                 &&
                                 (i == lea_has_preset_record_list.count - 1))
                        {
                            os_queue_in(&lea_has_preset_record_list, p_new_preset_record);
                            break;
                        }
                    }
                }

                if (!is_connect)
                {
                    app_lea_has_save_preset_change_idx(GENERIC_UPDATE, p_new_preset_record->has_preset_record.index);
                }

                uint8_t prev_insex = app_lea_has_get_prev_preset_idx(p_new_preset_record->has_preset_record.index);
                uint8_t *p_data = NULL;
                p_data = malloc(length + 3);
                if (p_data != NULL)
                {
                    p_data[0] = prev_insex;
                    p_data[1] = idx;
                    p_data[2] = preset_prop;
                    memcpy(&p_data[3], p_value, length);
                    has_send_preset_change_data_all(GENERIC_UPDATE, length + 3, p_data, true);
                    free(p_data);
                }
            }
        }
        break;

    case PRESET_CHANGE_DELETE:
        {
            T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = app_lea_has_find_preset_record(idx);
            if (p_preset_record != NULL)
            {
                os_queue_delete(&lea_has_preset_record_list, p_preset_record);
                free(p_preset_record->has_preset_record.p_name);
                free(p_preset_record);
                if (!is_connect)
                {
                    app_lea_has_save_preset_change_idx(PRESET_RECORD_DELETED, idx);
                }

                has_send_preset_change_data_all(PRESET_RECORD_DELETED, length, &idx, true);
            }
            else
            {
                goto error;
            }
        }
        break;

    case PRESET_CHANGE_AVAILABLE:
        {
            T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = app_lea_has_find_preset_record(idx);
            if (p_preset_record != NULL)
            {
                p_preset_record->has_preset_record.properties |= HAS_PRESET_PROPERTIES_AVAILABLE;

                if (!is_connect)
                {
                    app_lea_has_save_preset_change_idx(PRESET_RECORD_AVAILABLE, idx);
                }

                has_send_preset_change_data_all(PRESET_RECORD_AVAILABLE, sizeof(uint8_t), &idx, true);
            }
            else
            {
                goto error;
            }
        }
        break;

    case PRESET_CHANGE_UNAVAILABLE:
        {
            T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = app_lea_has_find_preset_record(idx);
            if (p_preset_record != NULL)
            {
                p_preset_record->has_preset_record.properties &= (~HAS_PRESET_PROPERTIES_AVAILABLE);
                if (!is_connect)
                {
                    app_lea_has_save_preset_change_idx(PRESET_RECORD_UNAVAILABLE, idx);
                }

                has_send_preset_change_data_all(PRESET_RECORD_UNAVAILABLE, length, &idx, true);
            }
            else
            {
                goto error;
            }
        }
        break;

    case PRESET_CHANGE_NAME_CHANGE:
        {
            if ((p_value == NULL) || (length == 0) || (length > HAS_MAX_PRESET_NAME_LENGTH))
            {
                APP_PRINT_ERROR1("app_lea_has_modify_preset_record_list: invalid param type %d", type);
                return false;
            }

            T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = app_lea_has_find_preset_record(idx);
            if (p_preset_record != NULL)
            {
                free(p_preset_record->has_preset_record.p_name);
                p_preset_record->has_preset_record.p_name = malloc(length);

                if (p_preset_record->has_preset_record.p_name != NULL)
                {
                    memcpy(p_preset_record->has_preset_record.p_name, p_value, length);
                    p_preset_record->name_len = length;

                    if (!is_connect)
                    {
                        app_lea_has_save_preset_change_idx(GENERIC_UPDATE, idx);
                    }

                    uint8_t prev_insex = app_lea_has_get_prev_preset_idx(p_preset_record->has_preset_record.index);
                    uint8_t *p_data = NULL;
                    p_data = malloc(length + 3);
                    if (p_data != NULL)
                    {
                        p_data[0] = prev_insex;
                        p_data[1] = p_preset_record->has_preset_record.index;
                        p_data[2] = p_preset_record->has_preset_record.properties;
                        memcpy(&p_data[3], p_preset_record->has_preset_record.p_name, p_preset_record->name_len);
                        has_send_preset_change_data_all(GENERIC_UPDATE, length + 3, p_data, true);

                        free(p_data);
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                goto error;
            }
        }
        break;

    default:
        break;
    }

    return true;

error:
    APP_PRINT_ERROR1("app_lea_has_modify_preset_record_list: index not exist, type %d", type);
    return false;
}

bool app_lea_has_send_read_preset_rsp(uint16_t conn_handle, uint8_t preset_idx, uint8_t preset_num)
{
    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset_record = NULL;
    bool is_last = false;

    if (lea_has_preset_record_list.count == 0)
    {
        return false;
    }

    p_preset_record = os_queue_peek(&lea_has_preset_record_list, lea_has_preset_record_list.count - 1);

    if (p_preset_record->has_preset_record.index < preset_idx)
    {
        return false;
    }

    uint8_t i = 0;
    uint8_t cur_num = 0;

    for (i = 0; i < lea_has_preset_record_list.count; i++)
    {
        p_preset_record = os_queue_peek(&lea_has_preset_record_list, i);

        if (p_preset_record->has_preset_record.index >= preset_idx)
        {
            break;
        }
    }

    for (uint8_t j = i; j < lea_has_preset_record_list.count; j++)
    {
        p_preset_record = os_queue_peek(&lea_has_preset_record_list, j);

        if (cur_num == preset_num - 1)
        {
            is_last = true;
        }

        if (j == lea_has_preset_record_list.count - 1)
        {
            is_last = true;
        }

        if (cur_num == preset_num)
        {
            break;
        }

        uint8_t *p_value = NULL;
        uint8_t length = p_preset_record->name_len + 2;
        p_value = malloc(length);
        if (p_value != NULL)
        {
            p_value[0] = p_preset_record->has_preset_record.index;
            p_value[1] = p_preset_record->has_preset_record.properties;
            memcpy(&p_value[2], p_preset_record->has_preset_record.p_name, p_preset_record->name_len);
            has_handle_read_preset_rsp(conn_handle, length, p_value, is_last);
            free(p_value);
            cur_num++;
        }
    }

    return true;
}

uint16_t app_lea_has_handle_set_active_idx_req(uint8_t index, bool remote_relay)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset = app_lea_has_find_preset_record(index);

    if (p_preset != NULL)
    {
        if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
        {
            has_update_active_preset_idx(index);
            if (remote_relay)
            {
#if F_APP_ERWS_SUPPORT
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HAS,
                                                       HAS_MSG_SYNC_LOCALLY,
                                                       &index, 1, REMOTE_TIMER_HIGH_PRECISION, 0, true);
                }
#endif
            }
            else
            {
                app_ha_hearing_set_prog_id(index - 1);
            }
        }
        else
        {
            cb_result = ATT_ERR_HAS_OP_PRESET_OPERATION_NOT_POSSIBLE;
        }
    }
    else
    {
        cb_result = (ATT_ERR | ATT_ERR_OUT_OF_RANGE);
    }

    return cb_result;
}

uint16_t app_lea_has_handle_set_next_active_idx_req(uint8_t cur_active_idx, bool remote_relay)
{
    bool is_found = false;
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset = NULL;

    uint8_t i = 0;
    for (i = 0; i < lea_has_preset_record_list.count; i++)
    {
        p_preset = os_queue_peek(&lea_has_preset_record_list, i);

        if (p_preset->has_preset_record.index == cur_active_idx)
        {
            break;
        }
    }

    if (i == lea_has_preset_record_list.count - 1)
    {
        for (uint8_t j = 0; j < lea_has_preset_record_list.count; j++)
        {
            p_preset = os_queue_peek(&lea_has_preset_record_list, j);

            if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
            {
                has_update_active_preset_idx(p_preset->has_preset_record.index);
                is_found = true;
                break;
            }
        }
    }
    else
    {
        for (uint8_t j = i + 1; j < lea_has_preset_record_list.count; j++)
        {
            p_preset = os_queue_peek(&lea_has_preset_record_list, j);

            if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
            {
                has_update_active_preset_idx(p_preset->has_preset_record.index);
                is_found = true;
                break;
            }
        }

        if (!is_found)
        {
            for (uint8_t j = 0; j < i; j++)
            {
                p_preset = os_queue_peek(&lea_has_preset_record_list, j);

                if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
                {
                    has_update_active_preset_idx(p_preset->has_preset_record.index);
                    is_found = true;
                    break;
                }
            }
        }

    }

    if (is_found)
    {
        uint8_t index = p_preset->has_preset_record.index;

        if (remote_relay)
        {
#if F_APP_ERWS_SUPPORT
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HAS,
                                                   HAS_MSG_SYNC_LOCALLY,
                                                   &index, 1, REMOTE_TIMER_HIGH_PRECISION, 0, true);
            }
#endif
        }
        else
        {
            app_ha_hearing_set_prog_id(index - 1);
        }
    }
    return cb_result;
}

uint16_t app_lea_has_handle_set_pre_active_idx_req(uint8_t cur_active_idx, bool remote_relay)
{
    bool is_found = false;
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset = NULL;

    uint8_t i = 0;
    for (i = 0; i < lea_has_preset_record_list.count; i++)
    {
        p_preset = os_queue_peek(&lea_has_preset_record_list, i);

        if (p_preset->has_preset_record.index == cur_active_idx)
        {
            break;
        }
    }

    if (i == 0)
    {
        for (int j = lea_has_preset_record_list.count - 1; j > 0; j--)
        {
            p_preset = os_queue_peek(&lea_has_preset_record_list, j);

            if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
            {
                has_update_active_preset_idx(p_preset->has_preset_record.index);
                is_found = true;
                break;
            }
        }
    }
    else
    {
        for (int j = i - 1; j >= 0; j--)
        {
            p_preset = os_queue_peek(&lea_has_preset_record_list, j);

            if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
            {
                has_update_active_preset_idx(p_preset->has_preset_record.index);
                is_found = true;
                break;
            }
        }

        if (!is_found)
        {
            for (int j = lea_has_preset_record_list.count - 1; j > i; j--)
            {
                p_preset = os_queue_peek(&lea_has_preset_record_list, j);

                if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_AVAILABLE)
                {
                    has_update_active_preset_idx(p_preset->has_preset_record.index);
                    is_found = true;
                    break;
                }
            }
        }

    }

    if (is_found)
    {
        uint8_t index = p_preset->has_preset_record.index;

        if (remote_relay)
        {
#if F_APP_ERWS_SUPPORT
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HAS,
                                                   HAS_MSG_SYNC_LOCALLY,
                                                   &index, 1, REMOTE_TIMER_HIGH_PRECISION, 0, true);
            }
#endif
        }
        else
        {
            app_ha_hearing_set_prog_id(index - 1);
        }
    }

    return cb_result;
}

uint16_t app_lea_has_handle_write_cp_req(T_HAS_CP_IND *p_write_req)
{
    bool handle = true;
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;

    switch (p_write_req->cp_op)
    {
    case HAS_CP_OP_READ_PRESETS_REQ:
        {
            if (!app_lea_has_send_read_preset_rsp(p_write_req->conn_handle,
                                                  p_write_req->index, p_write_req->preset_num))
            {
                cb_result = (ATT_ERR | ATT_ERR_OUT_OF_RANGE);
                has_clear_cp_flag();
            }
        }
        break;

    case HAS_CP_OP_WRITE_PRESET_NAME:
        {
            T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset = app_lea_has_find_preset_record(p_write_req->index);
            if (p_preset != NULL)
            {
                if (p_preset->has_preset_record.properties & HAS_PRESET_PROPERTIES_WRITABLE)
                {
                    if (!app_lea_has_modify_preset_record_list(PRESET_CHANGE_NAME_CHANGE,
                                                               p_write_req->index, 0,
                                                               p_write_req->name_len,
                                                               p_write_req->p_name, true))
                    {
                        cb_result = (ATT_ERR | ATT_ERR_OUT_OF_RANGE);
                    }
                }
                else
                {
                    cb_result = ATT_ERR_HAS_OP_WRITE_NAME_NOT_ALLOWED;
                }
            }
            else
            {
                cb_result = (ATT_ERR | ATT_ERR_OUT_OF_RANGE);
            }
        }
        break;

    case HAS_CP_OP_SET_ACTIVE_PRESET:
        {
            cb_result = app_lea_has_handle_set_active_idx_req(p_write_req->index, false);
        }
        break;

    case HAS_CP_OP_SET_NEXT_PRESET:
        {
            cb_result = app_lea_has_handle_set_next_active_idx_req(p_write_req->active_preset_idx, false);
        }
        break;

    case HAS_CP_OP_SET_PREVIOUS_PRESET:
        {
            cb_result = app_lea_has_handle_set_pre_active_idx_req(p_write_req->active_preset_idx, false);
        }
        break;

    case HAS_CP_OP_SET_ACTIVE_PRESET_SYNC_LOCAL:
        {
            cb_result = app_lea_has_handle_set_active_idx_req(p_write_req->index, true);
        }
        break;

    case HAS_CP_OP_SET_NEXT_PRESET_SYNC_LOCAL:
        {
            cb_result = app_lea_has_handle_set_next_active_idx_req(p_write_req->active_preset_idx, true);
        }
        break;

    case HAS_CP_OP_SET_PREVIOUS_PRESET_SYNC_LOCAL:
        {
            cb_result = app_lea_has_handle_set_pre_active_idx_req(p_write_req->active_preset_idx, true);
        }
        break;

    default:
        handle = false;
        break;
    }

    if (handle == true)
    {
        APP_PRINT_TRACE3("app_lea_has_handle_write_cp_req: cp_op 0x%04x, idx %d, active_idx %d",
                         p_write_req->cp_op, p_write_req->index, p_write_req->active_preset_idx);
    }

    return cb_result;
}

bool app_lea_has_handle_cccd_info(T_HAS_PENDING_PRESET_CHANGE *p_cccd_info)
{
    bool ret = true;
    uint8_t preset_num = lea_has_preset_change_list.count;
    T_LEA_HAS_PRESET_CHANGE_QUEUE_ELEM *p_change_idx = NULL;
    uint8_t offset = 0;

    while ((p_change_idx = os_queue_out(&lea_has_preset_change_list)) != NULL)
    {
        offset++;

        if (p_change_idx->change_id == GENERIC_UPDATE)
        {
            T_LEA_HAS_PRESET_QUEUE_ELEM *p_preset = app_lea_has_find_preset_record(p_change_idx->idx);
            if (p_preset != NULL)
            {
                uint8_t prev_index = app_lea_has_get_prev_preset_idx(p_preset->has_preset_record.index);
                uint8_t *p_data = NULL;
                p_data = malloc(p_preset->name_len + 3);
                if (p_data != NULL)
                {
                    p_data[0] = prev_index;
                    p_data[1] = p_preset->has_preset_record.index;
                    p_data[2] = p_preset->has_preset_record.properties;
                    memcpy(&p_data[3], &p_preset->has_preset_record.p_name, p_preset->name_len);

                    if (offset == preset_num)
                    {
                        if (has_send_preset_change_data(GENERIC_UPDATE, p_cccd_info->conn_handle,
                                                        p_preset->name_len + 3, p_data, true) == false)
                        {
                            ret = false;
                        }
                    }
                    else
                    {
                        if (has_send_preset_change_data(GENERIC_UPDATE, p_cccd_info->conn_handle,
                                                        p_preset->name_len + 3, p_data, false) == false)
                        {
                            ret = false;
                        }
                    }

                    free(p_data);
                    if (ret == false)
                    {
                        return ret;
                    }
                }
            }
        }
        else
        {
            if (offset == preset_num)
            {
                if (has_send_preset_change_data(p_change_idx->change_id, p_cccd_info->conn_handle,
                                                1, &p_change_idx->idx, true) == false)
                {
                    ret = false;
                }
            }
            else
            {
                if (has_send_preset_change_data(p_change_idx->change_id, p_cccd_info->conn_handle,
                                                1, &p_change_idx->idx, false) == false)
                {
                    ret = false;
                }
            }

            if (ret == false)
            {
                return ret;
            }
        }

        free(p_change_idx);
    }

    return ret;
}

#if F_APP_ERWS_SUPPORT
void app_lea_has_update_preset_idx_by_audio_conn(uint8_t id)
{
    uint8_t index = id + 1;

    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HAS,
                                           HAS_MSG_SYNC_BY_AUDIO_CONN,
                                           &index, 1, REMOTE_TIMER_HIGH_PRECISION, 0, true);
    }
    else
    {
        has_update_active_preset_idx(index);
    }
}

static void app_lea_has_bt_cback(T_BT_EVENT event_type, void *event_buf, uint16_t buf_len)
{
    switch (event_type)
    {
    case BT_EVENT_REMOTE_CONN_CMPL:
        {
            if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY)
            {
                uint8_t index = app_ha_hearing_get_active_prog_id() + 1;

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_HAS,
                                                   HAS_MSG_SYNC_LOCALLY,
                                                   &index, 1, REMOTE_TIMER_HIGH_PRECISION, 0, true);
            }
        }
        break;

    default:
        break;
    }
}

static uint16_t app_lea_has_relay_cback(uint8_t *buf, uint8_t msg_type, bool total)
{
    return app_relay_msg_pack(buf, msg_type, APP_MODULE_TYPE_UCA, 0, NULL, true, total);
}

static void app_lea_has_parse_cback(uint8_t msg_type, uint8_t *buf, uint16_t len,
                                    T_REMOTE_RELAY_STATUS status)
{
    APP_PRINT_TRACE2("app_lea_has_parse_cback: msg_type 0x%02X, status 0x%02X", msg_type, status);

    switch (msg_type)
    {
    case HAS_MSG_SYNC_LOCALLY:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t index = *(uint8_t *)buf;
                has_update_active_preset_idx(index);
                app_ha_hearing_set_prog_id(index - 1);
            }
        }
        break;

    case HAS_MSG_SYNC_BY_AUDIO_CONN:
        {
            if (status == REMOTE_RELAY_STATUS_SYNC_TOUT ||
                status == REMOTE_RELAY_STATUS_SYNC_EXPIRED ||
                status == REMOTE_RELAY_STATUS_SYNC_REF_CHANGED ||
                status == REMOTE_RELAY_STATUS_ASYNC_RCVD)
            {
                uint8_t index = *(uint8_t *)buf;
                has_update_active_preset_idx(index);
            }
        }
        break;

    default:
        break;
    }
}
#endif

void app_lea_has_preset_record_init(void)
{
    os_queue_init(&lea_has_preset_record_list);
    os_queue_init(&lea_has_preset_change_list);
#if F_APP_ERWS_SUPPORT
    bt_mgr_cback_register(app_lea_has_bt_cback);
    app_relay_cback_register(app_lea_has_relay_cback, app_lea_has_parse_cback,
                             APP_MODULE_TYPE_HAS, HAS_MSG_TOTAL);
#endif
}
#endif
