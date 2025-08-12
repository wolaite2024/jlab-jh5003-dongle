#if LE_AUDIO_CSIS_SUPPORT
#include "trace.h"
#include "le_audio_service.h"
#include "vector.h"
#include "os_mem.h"
#include "bap.h"
#include "app_usb_layer.h"
#include "le_csis_srv_service.h"
#include "csis_mgr.h"

static uint8_t le_csis_sirk[CSIS_SIRK_LEN] = {0x63, 0x68, 0x65, 0x6e,
                                              0x67, 0x2d, 0x63, 0x61,
                                              0x69, 0x73, 0x37, 0x38,
                                              0x32, 0x53, 0xe8, 0x10
                                             };

uint16_t le_csis_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_CSIS_READ_SIRK_IND:
        {
            T_CSIS_READ_SIRK_IND *p_data = (T_CSIS_READ_SIRK_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_CSIS_READ_SIRK_IND: conn_handle 0x%x, service_id %d, sirk_type %d",
                            p_data->conn_handle, p_data->service_id,
                            p_data->sirk_type);
        }
        break;

    case LE_AUDIO_MSG_CSIS_LOCK_STATE:
        {
            T_CSIS_LOCK_STATE *p_data = (T_CSIS_LOCK_STATE *)buf;
            APP_PRINT_INFO2("LE_AUDIO_MSG_CSIS_LOCK_STATE: service_id %d, lock_state %d",
                            p_data->service_id, p_data->lock_state);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

bool le_csis_srv_get_rsi(uint8_t *p_rsi)
{
    return csis_gen_rsi(le_csis_sirk, p_rsi);
}

void le_csis_srv_init(T_CAP_INIT_PARAMS *p_param)
{
    T_CSIS_SIRK_TYPE sirk_type = CSIS_SIRK_PLN;
    uint8_t feature = (SET_MEMBER_SIZE_EXIST | SET_MEMBER_RANK_EXIST);
    p_param->csis_num = 1;
    p_param->cas.enable = true;
    p_param->cas.csis_sirk_type = sirk_type;
    p_param->cas.csis_size = 2;
    p_param->cas.csis_rank = 1;
    p_param->cas.csis_feature = feature;
    p_param->cas.csis_sirk = le_csis_sirk;
}
#endif
