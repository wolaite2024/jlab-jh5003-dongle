#include "trace.h"
#include "le_unicast_gaming.h"
#include "codec_def.h"
#include "ble_audio_def.h"
#include "app_le_audio.h"
#include "app_timer.h"
#include "ual_dev_mgr.h"


#define AUDIO_MEDIA_PREFER_CFG              (CODEC_CFG_ITEM_48_3_BIT)
#define AUDIO_CONVERSATION_PREFER_CFG       (CODEC_CFG_ITEM_16_1_BIT)

#define LEA_TIMER_MODE_SWITCH_TIMEOUT       (300)

typedef enum
{
    LEA_GAMING_TIMER_MODE_SWITCH,
} T_LEA_GAMING_TIMER;

bool lea_gaming_mode = false;
bool lea_gaming_mode_request = false;

static uint8_t lea_gaming_timer_id = 0;

static uint8_t lea_gaming_timer_mode_switch = 0;


static void lea_handle_mode_switch_timeout(uint16_t param);

static void lea_gaming_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("lea_gaming_timeout_cb: timer_evt 0x%02x, param %d", timer_evt, param);

    switch (timer_evt)
    {
    case LEA_GAMING_TIMER_MODE_SWITCH:
        app_stop_timer(&lea_gaming_timer_mode_switch);
        lea_handle_mode_switch_timeout(param);
        break;

    default:
        break;
    }
}

bool le_unicast_is_gaming_mode(void)
{
    return lea_gaming_mode;
}

void le_unicast_set_gaming_mode(bool gaming)
{
    lea_gaming_mode = gaming;
    ble_set_gaming_mode(lea_gaming_mode);
}

static void lea_handle_mode_switch_timeout(uint16_t param)
{

    APP_PRINT_INFO2("lea_handle_mode_switch_timeout mode %x new_mode %x", lea_gaming_mode, param);
#if (DONGLE_LE_AUDIO == 1)
    if (lea_gaming_mode != param)
    {
        le_unicast_set_gaming_mode(param);
        if (le_unicast_stop_stream() == false)
        {
            APP_PRINT_WARN0("le_unicast_stop_stream false");
        }
    }
#endif
}

bool le_gaming_parse_metadata(uint8_t metadata_len, uint8_t *p_metadata, bool *gaming_mode)
{
    uint8_t remain_len = metadata_len;
    uint8_t *p_data = p_metadata;

    while (remain_len)
    {
        if (p_data[0] < 0x03)
        {
            return false;
        }
        if (p_data[1] != 0xFF)
        {
            remain_len -= (p_data[0] + 1);
            p_data += (p_data[0] + 1);
            continue;
        }

        if ((p_data[2] | p_data[3] << 8) != 0x005D)
        {
            remain_len -= (p_data[0] + 1);
            p_data += (p_data[0] + 1);
            continue;
        }

        if (p_data[4] == 0x02 && p_data[5] == VENDOR_DATA_TYPE_GAMING_MODE)
        {
            *gaming_mode = p_data[6] ? true : false;
            APP_PRINT_INFO1("le_gaming_parse_metadata gaming %x", *gaming_mode);
            return true;
        }
        remain_len -= (p_data[0] + 1);
        p_data += (p_data[0] + 1);
    }
    return false;
}

void app_switch_gaming_mode(uint8_t is_gaming_mode)
{
    extern uint8_t le_unicast_src_get_stream_state(void);
    if (le_unicast_src_get_stream_state())
    {
        if (lea_gaming_timer_mode_switch)
        {
            app_stop_timer(&lea_gaming_timer_mode_switch);
        }
        app_start_timer(&lea_gaming_timer_mode_switch, "mode_switch",
                        lea_gaming_timer_id, LEA_GAMING_TIMER_MODE_SWITCH, is_gaming_mode, false,
                        LEA_TIMER_MODE_SWITCH_TIMEOUT);
    }
    else
    {
        le_unicast_set_gaming_mode(is_gaming_mode);
    }
}

bool le_gaming_handle_pac_info_update(uint8_t *bd_addr, uint8_t bd_type, uint8_t role,
                                      uint16_t sup_freq, uint8_t chnl_cnts, uint16_t prefer_context,
                                      uint8_t metadata_len, uint8_t *p_metadata)
{
    APP_PRINT_INFO8("le_gaming_handle_pac_info_update addr %b type %x role %x sup_freq %x chnl_cnts %x prefer_context %x metadata_len %x, p_metadata %b",
                    TRACE_BDADDR(bd_addr), bd_type, role, sup_freq,
                    chnl_cnts, prefer_context, metadata_len, TRACE_BINARY(metadata_len, p_metadata));

    APP_PRINT_INFO1("le_gaming_handle_pac_info_update current_mode %x", lea_gaming_mode);

    if (role != SERVER_AUDIO_SINK)
    {
        return false;
    }

    if (le_gaming_parse_metadata(metadata_len, p_metadata, &lea_gaming_mode_request) == false)
    {
        return false;
    }

    /* do wrong, cis start stream will check lea_gaming_mode */
    app_switch_gaming_mode(lea_gaming_mode_request);

    return true;
}


void le_gaming_cfg_qos(T_AUDIO_STREAM_SESSION_HANDLE handle, uint8_t prefer_idx)
{
    uint8_t ret = 0;
    T_AUDIO_SESSION_QOS_CFG qos_cfg;
    T_CODEC_CFG codec_cfg;
    T_BAP_UNICAST_SESSION_INFO session_info;
    T_BAP_UNICAST_DEV_INFO *p_dev = NULL;

    if (handle == NULL)
    {
        ret = 1;
        goto fail;
    }

    if (!lea_gaming_mode)
    {
        ret = 2;
        goto fail;
    }

    if (!bap_unicast_audio_get_session_qos(handle, &qos_cfg))
    {
        ret = 3;
        goto fail;
    }

    codec_preferred_cfg_get((T_CODEC_CFG_ITEM)prefer_idx, &codec_cfg);
    qos_cfg.sdu_interval_s_m = qos_cfg.sdu_interval_m_s;
#if (LE_AUDIO_CIG_INTERLEAVED == 1)
    qos_cfg.packing = 1;    //interleaved
#else
    qos_cfg.packing = 0;
#endif
    if (codec_cfg.frame_duration == FRAME_DURATION_CFG_7_5_MS)
    {
        qos_cfg.latency_m_s = GAMING_DURATION_7_5_MS_QOS_LATENCY;
        qos_cfg.latency_s_m = GAMING_DURATION_7_5_MS_QOS_LATENCY;
    }
    else
    {
        qos_cfg.latency_m_s = GAMING_DURATION_10_MS_QOS_LATENCY;
        qos_cfg.latency_s_m = GAMING_DURATION_10_MS_QOS_LATENCY;
    }

    if (!bap_unicast_audio_cfg_session_qos(handle, &qos_cfg))
    {
        ret = 4;
        goto fail;
    }

    if (!bap_unicast_audio_get_session_info(handle, &session_info))
    {
        ret = 5;
        goto fail;
    }

    for (uint8_t i = 0; i < session_info.dev_num; i++)
    {
        p_dev = &session_info.dev_info[i];
        APP_PRINT_INFO4("le_gaming_cfg_qos: dev_info[%d]: dev_handle %p, conn_state %d, ase_num %d",
                        i, p_dev->dev_handle, p_dev->conn_state, p_dev->ase_num);

        for (uint8_t j = 0; j < p_dev->ase_num; j++)
        {
            T_AUDIO_ASE_QOS_CFG ase_qos_cfg;
            if (bap_unicast_audio_get_ase_qos(handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id,
                                              &ase_qos_cfg))
            {
                ase_qos_cfg.retransmission_number = GAMING_QOS_RTN;
                bap_unicast_audio_cfg_ase_qos(handle, p_dev->dev_handle, p_dev->ase_info[j].ase_id, &ase_qos_cfg);
            }
        }
    }

fail:
    APP_PRINT_INFO3("le_gaming_cfg_qos: fail! ret %d, handle %x, mode %x, ", -ret, handle,
                    lea_gaming_mode);

}

void lea_gaming_init(void)
{
    app_timer_reg_cb(lea_gaming_timeout_cb, &lea_gaming_timer_id);
}
