/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "stdlib_corecrt.h"
#include "os_mem.h"
#include "trace.h"
#include "eq.h"
#include "bt_a2dp.h"
#include "eq_utils.h"
#include "app_main.h"
#include "app_link_util.h"
#include "app_cmd.h"
#include "app_report.h"
#include "app_eq.h"
#include "app_cfg.h"
#include "app_audio_policy.h"
#include "app_bt_policy_api.h"
#include "app_anc.h"
#include "app_a2dp.h"
#include "app_ipc.h"

#if F_APP_LEA_SUPPORT
#include "app_lea_unicast_audio.h"
#endif

#if F_APP_LINEIN_SUPPORT
#include "app_line_in.h"
#endif

#if F_APP_USER_EQ_SUPPORT
#include "errno.h"
#endif

#if F_APP_USB_AUDIO_SUPPORT
#include "app_usb_audio.h"
#endif

#if F_APP_VOICE_SPK_EQ_SUPPORT
#include "app_hfp.h"
#include "eq_ext_ftl.h"
#endif

#if F_APP_EQ_COEFF_SUPPORT
#include "pm.h"
#endif

// for EQ setting
#define BOTH_SIDE_ADJUST        0x02

#define LO_WORD(x)              ((uint8_t)(x))
#define HI_WORD(x)              ((uint8_t)((x & 0xFF00) >> 8))

#if F_APP_EQ_COEFF_SUPPORT
#define MAX_CPU_FREQ            100
#endif

#if F_APP_USER_EQ_SUPPORT
#define USER_EQ_VERSION             0x00
#define USER_EQ_NUM_2M_FLASH_SIZE   6
#define USER_EQ_NUM_4M_FLASH_SIZE   20
#define MAX_TOTLA_USER_EQ_NUM       20
#define MAX_RELAY_SIZE              250

#if F_APP_ERWS_SUPPORT
uint16_t spk_user_eq_idx_waiting_sync;

#if F_APP_APT_SUPPORT
uint16_t mic_user_eq_idx_waiting_sync;
#endif

#endif  /* F_APP_ERWS_SUPPORT */

#if F_APP_EQ_COEFF_SUPPORT
typedef struct
{
    uint8_t val[5];

    /*total 5 bytes, but compiler would alignment to 6 bytes
    so claim 5 bytes and process it ourself. The format as below

    uint16_t biquad_type: 3;
    uint16_t q: 10;
    int16_t gain : 12;
    uint16_t freq : 15;*/
} T_APP_EQ_INFO_DATA;

typedef struct
{
    uint16_t stage_num: 4;
    uint16_t sample_freq: 4;
    uint16_t rsv: 8;

    int16_t global_gain;

    T_APP_EQ_INFO_DATA eq_info[10];
} T_APP_EQ_INFO;
#endif  /* F_APP_EQ_COEFF_SUPPORT */


#endif  /* F_APP_USER_EQ_SUPPORT */

typedef enum
{
    SAVE_EQ_FIELD_ONLY_APPLY      = 0x0,
    SAVE_EQ_FIELD_APPLY_AND_SAVE  = 0x1,
    SAVE_EQ_FIELD_ONLY_SAVE       = 0x2,
} T_APP_EQ_SAVE_EQ_FIELD;

bool app_eq_get_link_info(uint8_t cmd_path, uint8_t app_idx,
                          T_AUDIO_EQ_REPORT_DATA *eq_report_data);
bool app_eq_report_eq_frame(T_AUDIO_EQ_REPORT_DATA *eq_report_data);
static bool app_eq_report_get_eq_extend_info(T_AUDIO_EQ_INFO *p_eq_info);
static uint16_t app_eq_dsp_param_get(T_EQ_TYPE eq_type, T_EQ_STREAM_TYPE stream_type,
                                     uint8_t eq_mode, uint8_t index, void *data, uint16_t len, T_EQ_DATA_DEST eq_data_dest,
                                     uint32_t sample_rate);

#if F_APP_USER_EQ_SUPPORT
uint16_t app_eq_get_user_eq_size(void);
void app_eq_check_user_eq(void);

#if F_APP_ERWS_SUPPORT
bool app_eq_relay_sync_user_eq_msg(T_EQ_TYPE eq_type, uint8_t eq_idx,
                                   T_EQ_SYNC_ACTION sync_eq_action, uint16_t offset, uint8_t *eq_data, uint32_t len);
#endif

#endif

uint16_t app_eq_get_supported_sample_rate(void)
{
    uint16_t supported_sample_rate = BIT(AUDIO_EQ_SAMPLE_RATE_48KHZ) |
                                     BIT(AUDIO_EQ_SAMPLE_RATE_44_1KHZ);

    return supported_sample_rate;
}

uint32_t app_eq_get_sample_rate_value(uint8_t idx)
{
    uint32_t sample_rate_value[AUDIO_EQ_SAMPLE_RATE_MAX] = {8000, 16000, 32000, 44100, 48000};

    if (idx < AUDIO_EQ_SAMPLE_RATE_MAX)
    {
        return sample_rate_value[idx];
    }

    return 0;
}

static void app_eq_factory_reset_cback(uint32_t event, void *msg)
{
    switch (event)
    {
    case APP_DEVICE_IPC_EVT_FACTORY_RESET:
        {
#if F_APP_USER_EQ_SUPPORT
            app_eq_reset_all_user_eq();
#endif
        }
        break;

    default:
        break;
    }
}

void app_eq_init(void)
{
    uint16_t len = 0;
    app_db.max_eq_len = 0;

    if (eq_utils_init())
    {
        len = EQ_GROUP_NUM * (STAGE_NUM_SIZE + EXT_STAGE_NUM_SIZE_VER_2) + MCU_TO_SDK_CMD_HDR_SIZE +
              PUBLIC_VALUE_SIZE + EXT_PUB_VALUE_SIZE;//262

        app_db.max_eq_len = len;

#if F_APP_EXT_MIC_SWITCH_SUPPORT
        static bool eq_ipc_subscribe_flg = false;

        if (eq_ipc_subscribe_flg == false)
        {
            app_ipc_subscribe(APP_DEVICE_IPC_TOPIC, app_eq_factory_reset_cback);
            eq_ipc_subscribe_flg = true;
        }
#else
        app_ipc_subscribe(APP_DEVICE_IPC_TOPIC, app_eq_factory_reset_cback);
#endif

        app_eq_idx_check_accord_mode();

#if F_APP_USER_EQ_SUPPORT
        T_EQ_USER_EQ_HEADER eq_header;

        if (eq_utils_load_eq_from_ftl(0, (uint8_t *)&eq_header, sizeof(T_EQ_USER_EQ_HEADER)) == ENOF)
        {
            app_eq_reset_all_user_eq();
        }
        else
        {
            app_eq_check_user_eq();
        }
#endif
    }
}

void app_eq_deinit(void)
{
    app_db.max_eq_len = 0;
    eq_utils_deinit();
}

#if F_APP_LEA_SUPPORT
static void app_eq_media_eq_set(T_AUDIO_EFFECT_INSTANCE eq_instance, uint8_t *dynamic_eq_buf,
                                uint16_t eq_len)
{
    if (mtc_get_btmode() == MULTI_PRO_BT_BREDR)
    {
        eq_set(eq_instance, dynamic_eq_buf, eq_len);
    }
    else
    {
        eq_set(app_lea_uca_get_eq_instance(), dynamic_eq_buf, eq_len);
    }
}

static void app_eq_media_eq_enable(T_APP_BR_LINK *p_link)
{
    if (mtc_get_btmode() == MULTI_PRO_BT_BREDR && p_link != NULL)
    {
        app_eq_audio_eq_enable(&p_link->eq_instance, &p_link->audio_eq_enabled);
    }
    else if (mtc_get_btmode() == MULTI_PRO_BT_BREDR && app_lea_uca_get_eq_instance() != NULL)
    {
        app_eq_audio_eq_enable(app_lea_uca_p_eq_instance(), app_lea_uca_get_eq_abled());
    }
}
#endif

bool app_eq_index_set(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index)
{
    uint8_t eq_num;
    uint16_t eq_len;
    uint32_t sample_rate;
    bool ret = false;
    uint8_t *dynamic_eq_buf = calloc(1, app_db.max_eq_len);
    T_EQ_STREAM_TYPE stream_type = EQ_STREAM_TYPE_AUDIO;
    T_APP_BR_LINK *link = &app_db.br_link[app_a2dp_get_active_idx()];
    T_AUDIO_EFFECT_INSTANCE eq_instance;

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (mode == VOICE_SPK_MODE)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_16KHZ;
        stream_type = EQ_STREAM_TYPE_VOICE;
        link = &app_db.br_link[app_hfp_get_active_idx()];
        eq_instance = link->voice_spk_eq_instance;
    }
    else
#endif
    {
        sample_rate = app_eq_sample_rate_get();
        eq_instance = link->eq_instance;
    }

    if (dynamic_eq_buf != NULL)
    {
        eq_num = eq_utils_num_get(eq_type, mode);

        APP_PRINT_INFO4("app_eq_index_set: eq_type %d, mode %d, eq_num %d, index 0x%02x",
                        eq_type, mode, eq_num, index);

        if (eq_num == 0)
        {
            //when len < 0x10, has eq off effect
            eq_len = 0x05;
        }
        else
        {
            eq_len = app_eq_dsp_param_get(eq_type, stream_type, mode, index, dynamic_eq_buf,
                                          app_db.max_eq_len, EQ_DATA_TO_DSP, sample_rate);
        }

        if (eq_type == SPK_SW_EQ)
        {
#if F_APP_LINEIN_SUPPORT
            if (app_line_in_plug_state_get())
            {
                eq_set(app_db.line_in_eq_instance, dynamic_eq_buf, eq_len);
            }
            else
#endif
            {
#if F_APP_USB_AUDIO_SUPPORT
                if (app_usb_audio_get_ds_track_state() == AUDIO_TRACK_STATE_STARTED)
                {
                    eq_set((T_AUDIO_EFFECT_INSTANCE) app_usb_audio_get_eq_instance(), dynamic_eq_buf, eq_len);
                }
                else
#endif
                {
#if F_APP_LEA_SUPPORT
                    app_eq_media_eq_set(eq_instance, dynamic_eq_buf, eq_len);
#else
                    eq_set(eq_instance, dynamic_eq_buf, eq_len);
#endif
                }
            }

            app_cfg_nv.eq_idx = index;
            ret = true;
        }
        else
        {
            if (index <= eq_num && eq_len != 0)
            {
                eq_set(app_db.apt_eq_instance, dynamic_eq_buf, eq_len);
                app_cfg_nv.apt_eq_idx = index ;
                ret = true;
            }
        }

        free(dynamic_eq_buf);
    }
    else
    {
        APP_PRINT_ERROR0("app_eq_index_set: fail");
    }

    return ret;
}

bool app_eq_param_set(uint8_t eq_mode, uint8_t index, void *data, uint16_t len)
{
    uint8_t *buf;

    APP_PRINT_TRACE4("app_eq_param_set: eq_mode %d, index %u, data %p, len 0x%04x", eq_mode, index,
                     data, len);

    buf = data;

    if ((buf != NULL) && (len != 0))
    {
        if (app_db.sw_eq_type == SPK_SW_EQ)
        {
            T_APP_BR_LINK *link = NULL;
            T_AUDIO_EFFECT_INSTANCE eq_instance;

#if F_APP_VOICE_SPK_EQ_SUPPORT
            if (eq_mode == VOICE_SPK_MODE)
            {
                link = &app_db.br_link[app_hfp_get_active_idx()];
                eq_instance = link->voice_spk_eq_instance;
            }
            else
#endif
            {
#if F_APP_USB_AUDIO_SUPPORT
                if (app_usb_audio_get_ds_track_state() == AUDIO_TRACK_STATE_STARTED)
                {
                    eq_set((T_AUDIO_EFFECT_INSTANCE) app_usb_audio_get_eq_instance(), buf, len);
                }
                else
#endif
                {

                    link = &app_db.br_link[app_a2dp_get_active_idx()];
                    eq_instance = link->eq_instance;
                }
            }

#if F_APP_LEA_SUPPORT
            app_eq_media_eq_set(eq_instance, buf, len);
#else
            if (eq_instance != NULL)
            {
                eq_set(eq_instance, buf, len);
            }
#endif
            app_cfg_nv.eq_idx = index;
        }
        else
        {
            if (app_db.apt_eq_instance != NULL)
            {
                eq_set(app_db.apt_eq_instance, buf, len);
            }

            app_cfg_nv.apt_eq_idx = index;
        }

        return true;
    }

    return false;
}

#if F_APP_USER_EQ_SUPPORT
uint16_t app_eq_set_bits(uint8_t n)
{
    uint16_t val = 0;

    for (uint8_t i = 0; i < n; i++)
    {
        val |= BIT(i);
    }

    return val;
}

uint16_t app_eq_get_user_eq_size(void)
{
    uint16_t supported_sample_rate_num = __builtin_popcount(app_eq_get_supported_sample_rate());
    uint16_t eq_size = sizeof(T_EQ_USER_EQ_DATA) * supported_sample_rate_num +
                       sizeof(T_EQ_USER_EQ_INFO);

    return eq_size;

}

uint16_t app_eq_get_user_eq_info_offset(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index,
                                        uint8_t user_eq_spk_eq_num)
{
    uint16_t offset = 0;
    uint8_t eq_index = eq_utils_original_eq_index_get(eq_type, mode, index);
    uint16_t eq_size = app_eq_get_user_eq_size();

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (mode == VOICE_SPK_MODE)
    {
        if (eq_type == SPK_SW_EQ)
        {
            offset = AUDIO_AND_APT_EQ_SIZE;
        }
    }
    else
#endif
    {
        offset = sizeof(T_EQ_USER_EQ_HEADER) + (eq_index * eq_size);

        if (eq_type == MIC_SW_EQ)
        {
            offset += (user_eq_spk_eq_num * eq_size);
        }
    }

    return offset;
}

uint16_t app_eq_get_user_eq_data_offset(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index,
                                        uint8_t sample_rate)
{
    uint16_t offset = 0;
    uint8_t eq_index = eq_utils_original_eq_index_get(eq_type, mode, index);
    uint32_t samepl_rate_offset;
    uint16_t supported_sample_rate = app_eq_get_supported_sample_rate();
    uint16_t eq_size = app_eq_get_user_eq_size();

    samepl_rate_offset = __builtin_popcount(supported_sample_rate & (BIT(sample_rate) - 1));

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (mode == VOICE_SPK_MODE)
    {
        if (eq_type == SPK_SW_EQ)
        {
            offset = AUDIO_AND_APT_EQ_SIZE + sizeof(T_EQ_USER_EQ_INFO);
        }
    }
    else
#endif
    {
        offset = sizeof(T_EQ_USER_EQ_HEADER) + (eq_index * eq_size) +
                 (samepl_rate_offset * sizeof(T_EQ_USER_EQ_DATA)) + sizeof(T_EQ_USER_EQ_INFO);

        if (eq_type == MIC_SW_EQ)
        {
            offset += (app_cfg_const.user_eq_spk_eq_num * eq_size);
        }
    }

    return offset;
}

void app_eq_move_mic_user_eq(uint8_t mic_eq_idx, uint8_t old_spk_user_eq_num,
                             uint8_t new_spk_user_eq_num)
{
    uint16_t eq_size = app_eq_get_user_eq_size();
    uint16_t offset;
    uint8_t *eq_data = NULL;

    offset = app_eq_get_user_eq_info_offset(MIC_SW_EQ, EQ_MODE_NULL, mic_eq_idx, old_spk_user_eq_num);

    eq_data = calloc(eq_size, 1);
    if (eq_data == NULL)
    {
        goto exit;
    }

    if (eq_utils_load_eq_from_ftl(offset, eq_data, eq_size) == 0)
    {
        offset = app_eq_get_user_eq_info_offset(MIC_SW_EQ, EQ_MODE_NULL, mic_eq_idx, new_spk_user_eq_num);
        eq_utils_save_eq_to_ftl(offset, (uint8_t *)eq_data, eq_size);
    }

exit:
    if (eq_data)
    {
        free(eq_data);
    }
}

void app_eq_adjust_user_eq(uint8_t old_spk_user_eq_num, uint8_t old_mic_user_eq_num)
{
    uint8_t new_spk_user_eq_num = app_cfg_const.user_eq_spk_eq_num;
    uint8_t new_mic_user_eq_num = app_cfg_const.user_eq_mic_eq_num;
    uint8_t min_mic_num = (new_mic_user_eq_num > old_mic_user_eq_num) ? old_mic_user_eq_num :
                          new_mic_user_eq_num;
    uint8_t n;

    if (new_spk_user_eq_num > old_spk_user_eq_num)
    {
        for (n = min_mic_num; n > 0; n--)
        {
            app_eq_move_mic_user_eq(n - 1, old_spk_user_eq_num, new_spk_user_eq_num);
        }
    }
    else if (new_spk_user_eq_num < old_spk_user_eq_num)
    {
        for (n = 0; n < min_mic_num; n++)
        {
            app_eq_move_mic_user_eq(n, old_spk_user_eq_num, new_spk_user_eq_num);
        }
    }

    for (n = old_spk_user_eq_num; n < new_spk_user_eq_num; n++)
    {
        app_eq_reset_user_eq(SPK_SW_EQ, EQ_MODE_NULL, n);
    }

    for (n = old_mic_user_eq_num; n < new_mic_user_eq_num; n++)
    {
        app_eq_reset_user_eq(MIC_SW_EQ, EQ_MODE_NULL, n);
    }
}

void app_eq_check_user_eq(void)
{
    T_EQ_USER_EQ_HEADER eq_header;

    if (eq_utils_load_eq_from_ftl(0, (uint8_t *)&eq_header, sizeof(T_EQ_USER_EQ_HEADER)) == 0)
    {
        if (eq_header.supported_sample_rate != app_eq_get_supported_sample_rate())
        {
            /* the supported sample rate are different, reset all eq */
            app_eq_reset_all_user_eq();
        }
        else
        {
            if (eq_header.spk_user_eq_num != app_cfg_const.user_eq_spk_eq_num ||
                eq_header.mic_user_eq_num != app_cfg_const.user_eq_mic_eq_num)
            {
                app_eq_adjust_user_eq(eq_header.spk_user_eq_num, eq_header.mic_user_eq_num);

                eq_header.spk_user_eq_num = app_cfg_const.user_eq_spk_eq_num;
                eq_header.mic_user_eq_num = app_cfg_const.user_eq_mic_eq_num;
                eq_utils_save_eq_to_ftl(0, (uint8_t *) &eq_header, sizeof(T_EQ_USER_EQ_HEADER));
            }
        }
    }
}

bool app_eq_is_valid_user_eq_index(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index)
{
    uint8_t eq_index = eq_utils_original_eq_index_get(eq_type, mode, index);
    uint8_t user_eq_num = 0;
    bool ret = true;

    user_eq_num = (eq_type == SPK_SW_EQ) ? app_cfg_const.user_eq_spk_eq_num :
                  app_cfg_const.user_eq_mic_eq_num;

    if (user_eq_num <= eq_index)
    {
        ret = false;
    }

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (eq_type == SPK_SW_EQ && mode == VOICE_SPK_MODE)
    {
        ret = true;
    }
#endif

    return ret;
}

bool app_eq_reset_user_eq(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index)
{
    bool ret = true;
    uint8_t error = 0;
    uint32_t offset = 0;
    uint8_t eq_index = eq_utils_original_eq_index_get(eq_type, mode, index);
    uint16_t supported_sample_rate = app_eq_get_supported_sample_rate();
    T_EQ_USER_EQ_DATA eq_data = {0};
    T_EQ_USER_EQ_INFO eq_info = {0};
    uint8_t saved_eq_mode = EQ_MODE_NULL;

    if (!app_eq_is_valid_user_eq_index(eq_type, mode, index))
    {
        error = 1;
        ret = false;
        goto exit;
    }

    eq_info.eq_type = eq_type;
    eq_info.eq_idx = eq_index;
    eq_info.eq_info_len = 0;

    offset = app_eq_get_user_eq_info_offset(eq_type, mode, index, app_cfg_const.user_eq_spk_eq_num);
    eq_utils_save_eq_to_ftl(offset, (uint8_t *) &eq_info, sizeof(T_EQ_USER_EQ_INFO));

    eq_data.eq_data_len = 0;

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (mode == VOICE_SPK_MODE)
    {
        supported_sample_rate = AUDIO_EQ_SAMPLE_RATE_16KHZ;
        saved_eq_mode = VOICE_SPK_MODE;
    }
#endif

    for (uint8_t i = 0; i < AUDIO_EQ_SAMPLE_RATE_MAX; i++)
    {
        if (supported_sample_rate & BIT(i))
        {
            eq_data.sample_rate = i;
            offset = app_eq_get_user_eq_data_offset(eq_type, saved_eq_mode, eq_index, i);
            eq_utils_save_eq_to_ftl(offset, (uint8_t *) &eq_data, sizeof(T_EQ_USER_EQ_DATA));
        }
    }

exit:

    APP_PRINT_TRACE4("app_eq_reset_user_eq: type %d, idx %d, ret %d, %d", eq_type, eq_index, ret,
                     error);
    return ret;
}

void app_eq_reset_all_user_eq(void)
{
    uint8_t eq_index = 0;
    T_EQ_USER_EQ_HEADER eq_header = {0};

    eq_header.supported_sample_rate = app_eq_get_supported_sample_rate();
    eq_header.spk_user_eq_num = app_cfg_const.user_eq_spk_eq_num;
    eq_header.mic_user_eq_num = app_cfg_const.user_eq_mic_eq_num;
    eq_header.user_eq_version = USER_EQ_VERSION;

    eq_utils_save_eq_to_ftl(0, (uint8_t *) &eq_header, sizeof(T_EQ_USER_EQ_HEADER));

    for (eq_index = 0; eq_index < app_cfg_const.user_eq_spk_eq_num; eq_index++)
    {
        app_eq_reset_user_eq(SPK_SW_EQ, EQ_MODE_NULL, eq_index);
    }

#if F_APP_APT_SUPPORT
    for (eq_index = 0; eq_index < app_cfg_const.user_eq_mic_eq_num; eq_index++)
    {
        app_eq_reset_user_eq(MIC_SW_EQ, EQ_MODE_NULL, eq_index);
    }
#endif

#if F_APP_VOICE_SPK_EQ_SUPPORT
    app_eq_reset_user_eq(SPK_SW_EQ, VOICE_SPK_MODE, 0);
#endif


    APP_PRINT_TRACE0("app_eq_reset_all_user_eq: app_eq_reset_all_user_eq");
}

bool app_eq_save_user_eq_to_ftl(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index, uint8_t sample_rate,
                                uint8_t eq_adjust_side, uint8_t *p_data, uint16_t eq_data_len, uint8_t *p_eq_info,
                                uint16_t eq_info_len)
{
    uint8_t error = 0;
    bool ret = true;
    uint8_t eq_index = eq_utils_original_eq_index_get(eq_type, mode, index);
    uint16_t eq_info_offset = 0;
    uint16_t eq_data_offset = 0;
    T_EQ_USER_EQ_INFO eq_info = {0};
    T_EQ_USER_EQ_DATA eq_data = {0};

    if (!app_eq_is_valid_user_eq_index(eq_type, mode, index))
    {
        ret = false;
        error = 1;
        goto exit;
    }

    if (eq_info_len > sizeof(eq_info.eq_info) ||
        eq_data_len > sizeof(eq_data.eq_data))
    {
        ret = false;
        error = 2;
        goto exit;
    }

    eq_info.eq_type = eq_type;
    eq_info.eq_idx = eq_index;
    eq_info.eq_info_len = eq_info_len;
    memcpy(eq_info.eq_info, p_eq_info, eq_info_len);
    eq_info_offset = app_eq_get_user_eq_info_offset(eq_type, mode, index,
                                                    app_cfg_const.user_eq_spk_eq_num);

    eq_data.sample_rate = sample_rate;
    eq_data.eq_data_len = eq_data_len;
    memcpy(eq_data.eq_data, p_data, eq_data_len);
    eq_data_offset = app_eq_get_user_eq_data_offset(eq_type, mode, index, sample_rate);

    if (eq_adjust_side == BOTH_SIDE_ADJUST || eq_adjust_side == app_cfg_const.bud_side)
    {
        eq_utils_save_eq_to_ftl(eq_info_offset, (uint8_t *)&eq_info, sizeof(T_EQ_USER_EQ_INFO));
        eq_utils_save_eq_to_ftl(eq_data_offset, (uint8_t *)&eq_data, sizeof(T_EQ_USER_EQ_DATA));
    }

#if F_APP_ERWS_SUPPORT
    if (eq_adjust_side == BOTH_SIDE_ADJUST || eq_adjust_side != app_cfg_const.bud_side)
    {
        app_eq_relay_sync_user_eq_msg(eq_type, eq_index, EQ_SYNC_USER_EQ, eq_info_offset,
                                      (uint8_t *)&eq_info, sizeof(T_EQ_USER_EQ_INFO));
        app_eq_relay_sync_user_eq_msg(eq_type, eq_index, EQ_SYNC_USER_EQ, eq_data_offset,
                                      (uint8_t *) &eq_data, sizeof(T_EQ_USER_EQ_DATA));
    }
#endif

exit:

    APP_PRINT_TRACE2("app_eq_save_user_eq_to_ftl: ret %d, %d", ret, error);
    return ret;
}

uint16_t app_eq_load_user_eq_from_ftl(T_EQ_TYPE eq_type, uint8_t mode, uint8_t index, void *p_data,
                                      uint16_t len, T_EQ_DATA_DEST eq_data_dest, uint32_t sample_rate)
{
    uint8_t error = 0;
    uint32_t offset = 0;
    uint32_t tmp = 0;
    uint16_t eq_len = 0;

    if (!app_eq_is_valid_user_eq_index(eq_type, mode, index))
    {
        error = 1;
        goto exit;
    }

    if (eq_data_dest == EQ_DATA_TO_PHONE)
    {
        T_EQ_USER_EQ_INFO eq_info = {0};

        offset = app_eq_get_user_eq_info_offset(eq_type, mode, index, app_cfg_const.user_eq_spk_eq_num);
        tmp = eq_utils_load_eq_from_ftl(offset, (uint8_t *)&eq_info, sizeof(T_EQ_USER_EQ_INFO));
        if (tmp != 0)
        {
            error = 2;
            goto exit;
        }

        if (eq_info.eq_info_len != 0)
        {
            eq_len = eq_info.eq_info_len;
            memcpy(p_data, eq_info.eq_info, eq_len);
        }
    }
    else
    {
        T_EQ_USER_EQ_DATA eq_data = {0};

        offset = app_eq_get_user_eq_data_offset(eq_type, mode, index, sample_rate);
        tmp = eq_utils_load_eq_from_ftl(offset, (uint8_t *)&eq_data, sizeof(T_EQ_USER_EQ_DATA));

        if (tmp != 0)
        {
            error = 3;
            goto exit;
        }

        if (eq_data.eq_data_len != 0)
        {
            eq_len = eq_data.eq_data_len;
            memcpy(p_data, eq_data.eq_data, eq_len);
        }
    }

exit:
    APP_PRINT_TRACE2("app_eq_load_user_eq_from_ftl: len %d, %d", eq_len, error);

    return eq_len;
}

void app_eq_reset_unsaved_user_eq(void)
{
    if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx))
    {
#if F_APP_ERWS_SUPPORT
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
        {
            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                               APP_REMOTE_MSG_SYNC_DEFAULT_EQ_INDEX, &app_cfg_nv.eq_idx, sizeof(uint8_t),
                                               REMOTE_TIMER_HIGH_PRECISION, 0,
                                               false);
        }
        else
#endif
        {
            app_eq_index_set(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);
        }
    }

#if F_APP_APT_SUPPORT
    if (app_eq_is_valid_user_eq_index(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx))
    {
#if F_APP_ERWS_SUPPORT
        if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
        {
            app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_EQ_DEFAULT_INDEX_SYNC,
                                               &app_cfg_nv.apt_eq_idx, sizeof(uint8_t), REMOTE_TIMER_HIGH_PRECISION,
                                               0, false);
        }
        else
#endif
        {
            app_eq_index_set(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);
        }
    }
#endif

    APP_PRINT_TRACE0("app_eq_reset_unsaved_user_eq: app_eq_reset_unsaved_user_eq");
}

#if F_APP_EQ_COEFF_SUPPORT
void app_eq_info_set(uint8_t eq_type, uint8_t index, int32_t stage_num, double global_gain,
                     uint32_t *freq, double *gain, double *q, T_EQ_FILTER_TYPE *biquad_type, bool is_save)
{
    uint32_t cpu_freq, original_cpu_freq;
    uint32_t current_sample_rate;
    T_EQ_COEFF result;
    double linear_gain[10];
    double linear_global_gain;
    uint8_t i;
    uint8_t mode = (eq_type == MIC_SW_EQ) ? APT_MODE : app_db.spk_eq_mode;

    /* adjust cpu freq to speed up calculation */
    original_cpu_freq = pm_cpu_freq_get();
    pm_cpu_freq_set(MAX_CPU_FREQ, &cpu_freq);

    app_eq_coeff_check_para(stage_num, &global_gain, freq, gain, q, biquad_type);

    for (i = 0; i < stage_num; i++)
    {
        linear_gain[i] = app_eq_coeff_db_to_linear(gain[i]);
    }

    linear_global_gain = app_eq_coeff_db_to_linear(global_gain);

    current_sample_rate = app_eq_sample_rate_get();
    app_eq_coeff_calculate(stage_num, linear_global_gain,
                           app_eq_get_sample_rate_value(current_sample_rate), freq, linear_gain, q, biquad_type, &result);

    app_db.sw_eq_type = eq_type;
    app_eq_param_set(app_db.spk_eq_mode, index, result.value, result.coeff_len);

    if (is_save)
    {
        uint16_t supported_sample_rate = app_eq_get_supported_sample_rate();
        T_APP_EQ_INFO eq_info;
        uint8_t *p_eq_info_data;
        int32_t tmp;

        eq_info.stage_num = stage_num;
        eq_info.sample_freq = AUDIO_EQ_SAMPLE_RATE_44_1KHZ;
        eq_info.rsv = 0;
        eq_info.global_gain = (int16_t)(global_gain * 100);

        for (i = 0; i < stage_num; i++)
        {
            p_eq_info_data = eq_info.eq_info[i].val;

            p_eq_info_data[0] = (biquad_type[i] & 0x3);

            tmp = (int32_t)(q[i] * 100);
            p_eq_info_data[0] |= (tmp << 3) & 0xF8;
            p_eq_info_data[1] = (tmp >> 5) & 0x1F;

            tmp = (int32_t)(gain[i] * 100);
            p_eq_info_data[1] |= (tmp << 5) & 0xE0;
            p_eq_info_data[2] = (tmp >> 3) & 0xFF;
            p_eq_info_data[3] = (tmp >> 11) & 0x1;

            p_eq_info_data[3] |= (freq[i] << 1) & 0xFE;
            p_eq_info_data[4] = (freq[i] >> 7) & 0xFF;
        }

        /* save the EQ coeff of current sample rate */
        app_eq_save_user_eq_to_ftl((T_EQ_TYPE)eq_type, mode, index, current_sample_rate, BOTH_SIDE_ADJUST,
                                   (uint8_t *)&result.value, result.coeff_len, (uint8_t *)&eq_info, sizeof(eq_info));

        for (i = AUDIO_EQ_SAMPLE_RATE_8KHZ; i < AUDIO_EQ_SAMPLE_RATE_MAX; i++)
        {
            if (i == current_sample_rate)
            {
                continue;
            }

            if (supported_sample_rate & BIT(i))
            {
                app_eq_coeff_calculate(stage_num, linear_global_gain, app_eq_get_sample_rate_value(i), freq,
                                       linear_gain, q, biquad_type, &result);

                app_eq_save_user_eq_to_ftl((T_EQ_TYPE)eq_type, mode, index, i, BOTH_SIDE_ADJUST,
                                           (uint8_t *)&result.value, result.coeff_len, (uint8_t *)&eq_info, sizeof(eq_info));
            }
        }
    }

    /* resume to original cpu freq */
    pm_cpu_freq_set(original_cpu_freq, &cpu_freq);
}
#endif

#if F_APP_ERWS_SUPPORT
bool app_eq_relay_sync_user_eq_msg(T_EQ_TYPE eq_type, uint8_t eq_idx,
                                   T_EQ_SYNC_ACTION sync_eq_action, uint16_t offset, uint8_t *eq_data, uint32_t len)
{
    uint8_t *buffer = NULL;
    bool ret = true;

    buffer = calloc(1, len + 5);
    if (buffer == NULL)
    {
        ret = false;
        goto exit;
    }

    buffer[0] = sync_eq_action;
    buffer[1] = eq_type;
    buffer[2] = eq_idx;
    buffer[3] = LO_WORD(offset);
    buffer[4] = HI_WORD(offset);

    memcpy(&buffer[5], eq_data, len);

    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_USER_EQ,
                                        buffer, len + 5);

    free(buffer);

exit:
    return ret;
}

void app_eq_process_sync_user_eq_when_b2b_connected(T_EQ_TYPE eq_type, uint8_t eq_idx,
                                                    uint16_t offset, uint8_t *eq_data, uint16_t eq_len)
{
    static uint8_t *final_data = NULL;
    uint16_t eq_size = app_eq_get_user_eq_size();
    uint16_t eq_offset = app_eq_get_user_eq_info_offset(eq_type, EQ_MODE_NULL, eq_idx,
                                                        app_cfg_const.user_eq_spk_eq_num);

    if (eq_offset == offset && eq_size == eq_len)
    {
        /* EQ data did not been split to multiple packet, save data directly */
        eq_utils_save_eq_to_ftl(offset, eq_data, eq_len);
    }
    else
    {
        if (final_data == NULL)
        {
            final_data = (uint8_t *) malloc(eq_size);
            if (!final_data)
            {
                return;
            }
        }

        memcpy_s(&final_data[offset - eq_offset], (eq_size - (offset - eq_offset)), eq_data, eq_len);

        if ((offset + eq_len) == (eq_offset + eq_size))
        {
            /* receiving final packet of this EQ data, save data */
            eq_utils_save_eq_to_ftl(eq_offset, final_data, eq_size);
            free(final_data);
            final_data = NULL;
        }
    }
}

void app_eq_continue_sync_user_eq_when_connected(bool is_first_time, T_EQ_TYPE last_eq_type,
                                                 uint8_t last_eq_idx, uint16_t last_offset, uint16_t last_len)
{
    uint16_t eq_size = app_eq_get_user_eq_size();
    T_EQ_TYPE eq_type = EQ_TYPE_MAX;
    uint8_t eq_idx = last_eq_idx;
    uint16_t eq_offset = app_eq_get_user_eq_info_offset(last_eq_type, EQ_MODE_NULL, last_eq_idx,
                                                        app_cfg_const.user_eq_spk_eq_num);
    uint16_t new_offset = last_offset + last_len;
    uint16_t len;

    if (!is_first_time && ((new_offset - eq_offset) < eq_size)) /* sync the remaining data of this eq */
    {
        if (eq_offset + eq_size - new_offset >= MAX_RELAY_SIZE)
        {
            len = MAX_RELAY_SIZE;
        }
        else
        {
            len = eq_offset + eq_size - new_offset;
        }

        eq_type = last_eq_type;
    }
    else /* sync next eq */
    {
        if (!is_first_time)
        {
            if (last_eq_type == SPK_SW_EQ)
            {
                spk_user_eq_idx_waiting_sync &= ~(BIT(last_eq_idx));
            }
#if F_APP_APT_SUPPORT
            else if (last_eq_type == MIC_SW_EQ)
            {
                mic_user_eq_idx_waiting_sync &= ~(BIT(last_eq_idx));
            }
#endif
        }

        eq_idx = eq_utils_original_eq_index_get(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);
        if (spk_user_eq_idx_waiting_sync & BIT(eq_idx)) /* sync current spk eq first */
        {
            eq_type = SPK_SW_EQ;
        }
        else
        {
#if F_APP_APT_SUPPORT
            eq_idx = eq_utils_original_eq_index_get(MIC_SW_EQ, EQ_MODE_NULL, app_cfg_nv.apt_eq_idx);

            if (mic_user_eq_idx_waiting_sync & BIT(eq_idx)) /* sync current mic eq first */
            {
                eq_type = MIC_SW_EQ;
            }
            else
#endif
            {
                if (spk_user_eq_idx_waiting_sync != 0)
                {
                    eq_idx = __builtin_ffs(spk_user_eq_idx_waiting_sync) - 1;
                    eq_type = SPK_SW_EQ;
                }
#if F_APP_APT_SUPPORT
                else if (mic_user_eq_idx_waiting_sync != 0)
                {
                    eq_idx = __builtin_ffs(mic_user_eq_idx_waiting_sync) - 1;
                    eq_type = MIC_SW_EQ;
                }
#endif
            }
        }

        if (eq_type != EQ_TYPE_MAX)
        {
            len = (eq_size >= MAX_RELAY_SIZE) ? MAX_RELAY_SIZE : eq_size;
            new_offset = app_eq_get_user_eq_info_offset(eq_type, EQ_MODE_NULL, eq_idx,
                                                        app_cfg_const.user_eq_spk_eq_num);
        }
    }

    if (eq_type != EQ_TYPE_MAX)
    {
        uint16_t offset;
        uint8_t eq_data[MAX_RELAY_SIZE];

        if (eq_utils_load_eq_from_ftl(new_offset, eq_data, len) == 0)
        {
            app_eq_relay_sync_user_eq_msg(eq_type, eq_idx, EQ_SYNC_USER_EQ_WHEN_B2B_CONNECTED, new_offset,
                                          eq_data, len);
        }
    }
}

void app_eq_sync_user_eq_when_connected(void)
{
#if F_APP_AUDIO_VOCIE_SPK_EQ_INDEPENDENT_CFG
    spk_user_eq_idx_waiting_sync = 0;
#else
    spk_user_eq_idx_waiting_sync = app_eq_set_bits(app_cfg_const.user_eq_spk_eq_num);
#endif

#if F_APP_APT_SUPPORT
    mic_user_eq_idx_waiting_sync = 0;

    if (app_cfg_const.normal_apt_support && !app_cfg_const.rws_apt_eq_adjust_separate)
    {
        if (eq_utils_num_get(MIC_SW_EQ, APT_MODE) != 0)
        {
            mic_user_eq_idx_waiting_sync = app_eq_set_bits(app_cfg_const.user_eq_mic_eq_num);
        }
    }
#endif

    app_eq_continue_sync_user_eq_when_connected(true, EQ_TYPE_MAX, 0, 0, 0);
}

#if F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
void app_eq_report_sec_eq_to_src(uint8_t cmd_path, uint8_t app_idx, uint16_t eq_len,
                                 uint8_t *eq_data)
{
    T_AUDIO_EQ_REPORT_DATA eq_report_data;

    if (app_eq_get_link_info(cmd_path, app_idx, &eq_report_data))
    {
        T_AUDIO_EQ_INFO *eq_info = eq_report_data.eq_info;

        eq_info->eq_data_len = eq_len;

        if (eq_info->eq_data_buf != NULL)
        {
            free(eq_info->eq_data_buf);
        }

        eq_info->eq_data_buf = malloc(app_db.max_eq_len);
        if (eq_info->eq_data_buf)
        {
            memcpy_s(eq_info->eq_data_buf, (app_db.max_eq_len), eq_data, eq_info->eq_data_len);
            app_eq_report_eq_frame(&eq_report_data);
        }
    }
}

void app_eq_report_sec_eq_to_pri(uint8_t eq_type, uint8_t eq_mode, uint8_t eq_idx, uint8_t cmd_path,
                                 uint8_t app_idx)
{
    T_AUDIO_EQ_INFO eq_info;

    memset(&eq_info, 0, sizeof(eq_info));

    eq_info.eq_mode = eq_mode;
    eq_info.eq_idx = eq_idx;
    eq_info.sw_eq_type = eq_type;
    eq_info.eq_data_buf = NULL;

    if (app_eq_report_get_eq_extend_info(&eq_info))
    {
        uint8_t *buffer = malloc(eq_info.eq_data_len + 7);

        if (buffer)
        {
            buffer[0] = EQ_SYNC_REPORT_SECONDARY_EQ_INFO;
            buffer[1] = eq_type;
            buffer[2] = eq_idx;
            buffer[3] = cmd_path;
            buffer[4] = app_idx;
            buffer[5] = LO_WORD(eq_info.eq_data_len);
            buffer[6] = HI_WORD(eq_info.eq_data_len);

            memcpy(&buffer[7], eq_info.eq_data_buf, eq_info.eq_data_len);
            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_USER_EQ,
                                                buffer, eq_info.eq_data_len + 7);
            free(buffer);
        }
    }

    if (eq_info.eq_data_buf)
    {
        free(eq_info.eq_data_buf);
    }
}
#endif  /* F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT */
#endif  /* F_APP_ERWS_SUPPORT */
#endif  /* F_APP_USER_EQ_SUPPORT */

static uint16_t app_eq_dsp_param_get(T_EQ_TYPE eq_type, T_EQ_STREAM_TYPE stream_type,
                                     uint8_t eq_mode, uint8_t index, void *data, uint16_t len, T_EQ_DATA_DEST eq_data_dest,
                                     uint32_t sample_rate)
{
    uint16_t eq_len = 0;

    APP_PRINT_INFO5("app_eq_dsp_param_get: eq_type %d, dsp cfg index %d, data %p, len %u, sample_rate %d",
                    eq_type, index, data, len, sample_rate);

#if F_APP_USER_EQ_SUPPORT
    if ((stream_type == EQ_STREAM_TYPE_AUDIO) || ((stream_type == EQ_STREAM_TYPE_VOICE) &&
                                                  (eq_type == SPK_SW_EQ)))
    {
        eq_len = app_eq_load_user_eq_from_ftl(eq_type, eq_mode, index, data, len, eq_data_dest,
                                              sample_rate);
    }

    if (eq_len == 0)
#endif
    {
        eq_len = eq_utils_param_get(eq_type, stream_type, eq_mode, index, data, len, eq_data_dest,
                                    sample_rate);
    }

    return eq_len;
}

void app_eq_audio_eq_enable(T_AUDIO_EFFECT_INSTANCE *eq_instance, bool *audio_eq_enabled)
{
    if (eq_instance && audio_eq_enabled)
    {
        if (!(*audio_eq_enabled))
        {
            eq_enable(*eq_instance);
            *audio_eq_enabled = true;
        }
    }
}

uint32_t app_eq_sample_rate_get(void)
{
    uint32_t sample_rate = 0;

#if F_APP_LINEIN_SUPPORT
    if (app_line_in_plug_state_get())
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_48KHZ;
        APP_PRINT_TRACE0("app_eq_sample_rate_get: line in 48K");

        return sample_rate;
    }
#endif

#if F_APP_USB_AUDIO_SUPPORT
    if (app_usb_audio_get_ds_track_state() == AUDIO_TRACK_STATE_STARTED)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_48KHZ;
        APP_PRINT_TRACE0("app_eq_sample_rate_get: usb audio 48K");

        return sample_rate;
    }
#endif

#if F_APP_LEA_SUPPORT
    if ((app_link_get_b2s_link_num() == 0) &&
        mtc_get_btmode() == MULTI_PRO_BT_BREDR)
#else
    if (app_link_get_b2s_link_num() == 0)
#endif
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_44_1KHZ;
        APP_PRINT_ERROR0("app_eq_sample_rate_get: address error, return default sample_rate");

        return sample_rate;
    }

    T_APP_BR_LINK *link = &app_db.br_link[app_a2dp_get_active_idx()];
    sample_rate = link->a2dp_codec_info.sampling_frequency;

    if (link->a2dp_codec_info.sampling_frequency == SAMPLE_RATE_44K)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_44_1KHZ;
    }
    else if (link->a2dp_codec_info.sampling_frequency == SAMPLE_RATE_48K)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_48KHZ;
    }
    else
    {
        /* return default value */
        sample_rate = AUDIO_EQ_SAMPLE_RATE_44_1KHZ;
    }

#if F_APP_LEA_SUPPORT
    if (mtc_get_btmode() != MULTI_PRO_BT_BREDR)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_48KHZ;
    }
#endif

    APP_PRINT_TRACE2("app_eq_sample_rate_get: codec_type 0x%x, sample_rate %d",
                     link->a2dp_codec_type, link->a2dp_codec_info.sampling_frequency);

    return sample_rate;
}

static bool app_eq_report_get_eq_extend_info(T_AUDIO_EQ_INFO *p_eq_info)
{
    bool ret = true;
    int err = 0;

    uint32_t sample_rate;
    uint16_t eq_len = 0;
    uint16_t eq_len_to_dsp = 0;
    uint8_t  *eq_data_temp = calloc(app_db.max_eq_len, sizeof(uint8_t));
    T_EQ_STREAM_TYPE stream_type = EQ_STREAM_TYPE_AUDIO;

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (p_eq_info->eq_mode == VOICE_SPK_MODE)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_16KHZ;
        stream_type = EQ_STREAM_TYPE_VOICE;
    }
    else
#endif
    {
        sample_rate = app_eq_sample_rate_get();
    }

    if (!eq_data_temp)
    {
        err = 1;
        ret = false;
        goto exit;
    }

    eq_len = app_eq_dsp_param_get((T_EQ_TYPE)p_eq_info->sw_eq_type, stream_type, p_eq_info->eq_mode,
                                  p_eq_info->eq_idx, eq_data_temp,  app_db.max_eq_len, EQ_DATA_TO_PHONE, sample_rate);

    if (eq_len == 0)
    {
        err = 2;
        ret = false;
        goto exit;
    }

    p_eq_info->eq_data_len = eq_len + EQ_SAMPLE_RATE_SIZE;

    if (p_eq_info->eq_data_buf != NULL)
    {
        free(p_eq_info->eq_data_buf);
    }

    p_eq_info->eq_data_buf = malloc(p_eq_info->eq_data_len);
    if (p_eq_info->eq_data_buf == NULL)
    {
        err = 3;
        ret = false;
        goto exit;
    }

    p_eq_info->eq_data_buf[0] = sample_rate; //3 - > 44.1K   4 -> 48k
    memcpy(&(p_eq_info->eq_data_buf)[1], &eq_data_temp[0], eq_len);

exit:
    if (eq_data_temp)
    {
        free(eq_data_temp);
    }

    APP_PRINT_TRACE2("app_eq_generate_eq_param_for_report: ret %d, err %d", ret, err);

    return ret;
}

void app_eq_report_abort_frame(T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    uint8_t abort_frame[10];
    uint16_t frame_len = 0x02;

    abort_frame[0] = eq_report_data->eq_info->eq_idx;
    abort_frame[1] = eq_report_data->eq_info->adjust_side;
    abort_frame[2] = eq_report_data->eq_info->sw_eq_type;
    abort_frame[3] = eq_report_data->eq_info->eq_mode;
    abort_frame[4] = AUDIO_EQ_FRAME_ABORT;
    abort_frame[5] = eq_report_data->eq_info->eq_seq;
    abort_frame[6] = (uint8_t)(frame_len & 0xFF);
    abort_frame[7] = (uint8_t)((frame_len >> 8) & 0xFF);
    abort_frame[8] = (uint8_t)(CMD_AUDIO_EQ_PARAM_GET & 0xFF);
    abort_frame[9] = (uint8_t)((CMD_AUDIO_EQ_PARAM_GET >> 8) & 0xFF);

    app_report_event(eq_report_data->cmd_path, EVENT_AUDIO_EQ_PARAM_REPORT, eq_report_data->id,
                     abort_frame, frame_len + EQ_ABORT_FRAME_HEADER_LEN);

    if (eq_report_data->eq_info->eq_data_buf != NULL)
    {
        free(eq_report_data->eq_info->eq_data_buf);
        eq_report_data->eq_info->eq_data_buf = NULL;
    }
}

bool app_eq_get_link_info(uint8_t cmd_path, uint8_t app_idx, T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    bool ret = false;

    eq_report_data->cmd_path = cmd_path;

    if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
    {
        T_APP_BR_LINK *p_br_link = (T_APP_BR_LINK *) &app_db.br_link[app_idx];

        if (p_br_link)
        {
            eq_report_data->eq_info = &p_br_link->audio_get_eq_info;
            eq_report_data->id = p_br_link->id;

            ret = true;
        }
    }
    else if (cmd_path == CMD_PATH_LE)
    {
        T_APP_LE_LINK *p_le_link = (T_APP_LE_LINK *) &app_db.le_link[app_idx];

        if (p_le_link)
        {
            eq_report_data->eq_info = &p_le_link->audio_get_eq_info;

            /* EQ: 3 is att header,  6 is trasmint service header */
            eq_report_data->max_frame_len = p_le_link->mtu_size - 3 - EQ_START_FRAME_HEADER_LEN - 6;
            eq_report_data->id = p_le_link->id;

            ret = true;
        }
    }

    return ret;
}

bool app_eq_report_eq_frame(T_AUDIO_EQ_REPORT_DATA *eq_report_data)
{
    T_AUDIO_EQ_INFO *p_eq_info = eq_report_data->eq_info;
    uint16_t frame_len = p_eq_info->eq_data_len;
    T_AUDIO_EQ_FRAME_TYPE eq_frame_type;
    uint8_t *frame;
    uint8_t frame_idx = 0;
    uint8_t total_frame_len = 0;

    if (eq_report_data->cmd_path == CMD_PATH_LE)
    {
        if (p_eq_info->eq_data_offset == 0)
        {
            if (p_eq_info->eq_data_len > eq_report_data->max_frame_len)
            {
                eq_frame_type = AUDIO_EQ_FRAME_START;
                frame_len = eq_report_data->max_frame_len;
            }
            else
            {
                eq_frame_type = AUDIO_EQ_FRAME_SINGLE;
            }
        }
        else
        {
            if (p_eq_info->eq_data_len > eq_report_data->max_frame_len)
            {
                eq_frame_type = AUDIO_EQ_FRAME_CONTINUE;
                frame_len = eq_report_data->max_frame_len;
            }
            else
            {
                eq_frame_type = AUDIO_EQ_FRAME_END;
            }
        }
    }
    else
    {
        eq_frame_type = AUDIO_EQ_FRAME_SINGLE;
    }

    if (eq_frame_type == AUDIO_EQ_FRAME_START)
    {
        total_frame_len = frame_len + EQ_START_FRAME_HEADER_LEN;
    }
    else
    {
        total_frame_len = frame_len + EQ_SINGLE_FRAME_HEADER_LEN;
    }

    frame = malloc(total_frame_len);

    if (frame == NULL)
    {
        app_eq_report_abort_frame(eq_report_data);
        return false;
    }

    frame[frame_idx++] = p_eq_info->eq_idx;
    frame[frame_idx++] = p_eq_info->adjust_side;
    frame[frame_idx++] = p_eq_info->sw_eq_type;
    frame[frame_idx++] = p_eq_info->eq_mode;
    frame[frame_idx++] = eq_frame_type;
    frame[frame_idx++] = p_eq_info->eq_seq;

    if (eq_frame_type == AUDIO_EQ_FRAME_START)
    {
        frame[frame_idx++] = (uint8_t)(p_eq_info->eq_data_len & 0xFF);/*the low byte of frame_len*/
        frame[frame_idx++] = (uint8_t)((p_eq_info->eq_data_len >> 8) & 0xFF);/*the high byte of frame_len*/
    }

    frame[frame_idx++] = (uint8_t)(frame_len & 0xFF);/*the low byte of frame_len*/
    frame[frame_idx++] = (uint8_t)((frame_len >> 8) & 0xFF);/*the high byte of frame_len*/

    memcpy(&frame[frame_idx], p_eq_info->eq_data_buf, frame_len);

    app_report_event(eq_report_data->cmd_path, EVENT_AUDIO_EQ_PARAM_REPORT, eq_report_data->id,
                     frame, total_frame_len);

    p_eq_info->eq_data_len -= frame_len;

    if (frame[3] == AUDIO_EQ_FRAME_START || frame[3] == AUDIO_EQ_FRAME_CONTINUE)
    {
        p_eq_info->eq_data_offset += frame_len;
        (p_eq_info->eq_seq)++;
    }
    else
    {
        if (p_eq_info->eq_data_buf != NULL)
        {
            free(p_eq_info->eq_data_buf);
            p_eq_info->eq_data_buf = NULL;
        }
    }

    free(frame);

    return true;
}

void app_eq_report_eq_param(uint8_t cmd_path, uint8_t app_idx)
{
    int8_t err = 0;
    T_AUDIO_EQ_REPORT_DATA eq_report_data;

    if (!app_eq_get_link_info(cmd_path, app_idx, &eq_report_data))
    {
        err = 1;
        goto exit;
    }

    if (eq_report_data.eq_info->eq_data_len > 0)
    {
        if (!app_eq_report_eq_frame(&eq_report_data))
        {
            err = 2;
            goto exit;
        }
    }

exit:

    APP_PRINT_TRACE3("app_eq_report_eq_param: cmd_path %d, len %d, ret %d", cmd_path,
                     eq_report_data.eq_info->eq_data_len, err);
}

void app_eq_report_terminate_param_report(uint8_t cmd_path, uint8_t app_idx)
{
    T_AUDIO_EQ_REPORT_DATA eq_report_data;

    int8_t err = 0;

    if (!app_eq_get_link_info(cmd_path, app_idx, &eq_report_data))
    {
        err = 1;
        goto exit;
    }

    app_eq_report_abort_frame(&eq_report_data);

exit:
    APP_PRINT_TRACE1("app_eq_report_abort: err %d", err);
}

void app_eq_idx_check_accord_mode(void)
{
#if F_APP_LINEIN_SUPPORT
    if ((app_db.spk_eq_mode == LINE_IN_MODE) &&
        (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
    {
        app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_line_in_mode_record;
    }
    else
#endif
    {
        if ((app_db.spk_eq_mode == GAMING_MODE) &&
            (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
        {
            app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_gaming_mode_record;
        }
#if F_APP_ANC_SUPPORT
        else if ((app_db.spk_eq_mode == ANC_MODE) &&
                 (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
        {
            app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_anc_mode_record;
        }
#endif
#if F_APP_VOICE_SPK_EQ_SUPPORT
        else if ((app_db.spk_eq_mode == VOICE_SPK_MODE) &&
                 (eq_utils_num_get(SPK_SW_EQ, app_db.spk_eq_mode) != 0))
        {
            app_cfg_nv.eq_idx = 0;
        }
#endif
        else if ((eq_utils_num_get(SPK_SW_EQ, NORMAL_MODE) != 0))
        {
            app_cfg_nv.eq_idx = app_cfg_nv.eq_idx_normal_mode_record;
        }
    }
}

void app_eq_sync_idx_accord_eq_mode(uint8_t eq_mode, uint8_t index)
{
    if (eq_mode == GAMING_MODE)
    {
        app_cfg_nv.eq_idx_gaming_mode_record = index;

#if F_APP_ERWS_SUPPORT
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_GAMING_RECORD_EQ_INDEX);
#endif
    }
    else if (eq_mode == NORMAL_MODE)
    {
        app_cfg_nv.eq_idx_normal_mode_record = index;

#if F_APP_ERWS_SUPPORT
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_NORMAL_RECORD_EQ_INDEX);
#endif
    }

#if F_APP_ANC_SUPPORT
    else if (eq_mode == ANC_MODE)
    {
        app_cfg_nv.eq_idx_anc_mode_record = index;

#if F_APP_ERWS_SUPPORT
        app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_ANC_RECORD_EQ_INDEX);
#endif
    }
#endif

#if F_APP_LINEIN_SUPPORT
    else if (eq_mode == LINE_IN_MODE)
    {
        /* do not need to relay due to only stereo support line-in */
        app_cfg_nv.eq_idx_line_in_mode_record = index;
    }
#endif
}

void app_eq_play_audio_eq_tone(void)
{
#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
    return;
#else

    uint8_t eq_index = eq_utils_original_eq_index_get(SPK_SW_EQ, app_db.spk_eq_mode, app_cfg_nv.eq_idx);

    app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_AUDIO_EQ_0 + eq_index),
                             false, true);
#endif
}

void app_eq_play_apt_eq_tone(void)
{
    uint8_t eq_index = eq_utils_original_eq_index_get(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);

#if CONFIG_REALTEK_APP_TEAMS_FEATURE_SUPPORT
#else
    app_audio_tone_type_play((T_APP_AUDIO_TONE_TYPE)(TONE_APT_EQ_0 + eq_index),
                             false, true);
#endif
}

T_AUDIO_EFFECT_INSTANCE app_eq_create(T_EQ_CONTENT_TYPE eq_content_type,
                                      T_EQ_STREAM_TYPE stream_type, T_EQ_TYPE eq_type, uint8_t eq_mode, uint8_t eq_index)
{
    uint32_t sample_rate;
    T_AUDIO_EFFECT_INSTANCE eq_instance = NULL;
    uint8_t eq_num = eq_utils_num_get(eq_type, eq_mode);

#if F_APP_VOICE_SPK_EQ_SUPPORT
    if (eq_content_type == EQ_CONTENT_TYPE_VOICE)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_16KHZ;
    }
    else if (eq_content_type == EQ_CONTENT_TYPE_RECORD)
    {
        sample_rate = AUDIO_EQ_SAMPLE_RATE_16KHZ;
    }
    else
#endif
    {
        sample_rate = app_eq_sample_rate_get();
    }

    if (eq_num != 0)
    {
        uint8_t *dynamic_eq_buf = calloc(1, app_db.max_eq_len);

        if (dynamic_eq_buf != NULL)
        {
            uint16_t eq_len = app_eq_dsp_param_get(eq_type, stream_type, eq_mode, eq_index, dynamic_eq_buf,
                                                   app_db.max_eq_len, EQ_DATA_TO_DSP, sample_rate);

            eq_instance = eq_create(eq_content_type, dynamic_eq_buf, eq_len);
            free(dynamic_eq_buf);
        }
        else
        {
            APP_PRINT_ERROR0("app_eq_create: fail");
        }
    }

    return eq_instance;
}

static uint8_t app_eq_get_stage_num_accord_eq_len(uint16_t info_len)
{
    uint16_t stage_num_from_cmd = 0;

    stage_num_from_cmd = (info_len - EQ_SAMPLE_RATE_SIZE - EQ_SDK_HEADER_SIZE - EQ_HEADER_SIZE -
                          EQ_INFO_HEADER_SIZE) /
                         (STAGE_NUM_SIZE +
                          EXT_STAGE_NUM_SIZE_VER_2);

    APP_PRINT_TRACE1("app_eq_get_stage_num_accord_eq_len %d", stage_num_from_cmd);

    return stage_num_from_cmd;
}

static bool app_eq_cmd_operate(uint8_t eq_mode, uint8_t eq_adjust_side, uint8_t is_play_eq_tone,
                               uint8_t eq_idx, uint8_t eq_len_to_dsp, uint8_t *cmd_ptr)
{
    bool ret = true;

    APP_PRINT_TRACE6("app_eq_cmd_operate: type:%d, mode:%d, side:%d, tone:%d, idx:%d, len:%d",
                     app_db.sw_eq_type, eq_mode, eq_adjust_side, is_play_eq_tone, eq_idx, eq_len_to_dsp);

#if F_APP_ERWS_SUPPORT
    app_relay_async_single(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_SW_EQ_TYPE);
#endif

    if (app_db.sw_eq_type == SPK_SW_EQ)
    {
        if (is_play_eq_tone)
        {
            app_cfg_nv.eq_idx = eq_idx;
            app_eq_play_audio_eq_tone();
        }

        if (eq_adjust_side == BOTH_SIDE_ADJUST)
        {
#if F_APP_ERWS_SUPPORT
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                uint16_t relay_data_len = eq_len_to_dsp + 1;
                uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

                relay_data[0] = eq_mode;
                memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                   relay_data, relay_data_len, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                free(relay_data);
            }
            else
#endif
            {
                T_APP_BR_LINK *p_link = &app_db.br_link[app_a2dp_get_active_idx()];

                app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);

#if F_APP_VOICE_SPK_EQ_SUPPORT
                if (eq_mode == VOICE_SPK_MODE)
                {
                    p_link = &app_db.br_link[app_hfp_get_active_idx()];
                }
#endif

#if F_APP_LEA_SUPPORT
                app_eq_media_eq_enable(p_link);
#else
                if (p_link)
                {
                    app_eq_audio_eq_enable(&p_link->eq_instance, &p_link->audio_eq_enabled);
                }
#endif
            }
        }
        else if (eq_adjust_side == app_cfg_const.bud_side)
        {
            T_APP_BR_LINK *p_link = &app_db.br_link[app_a2dp_get_active_idx()];

            app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);

#if F_APP_VOICE_SPK_EQ_SUPPORT
            if (eq_mode == VOICE_SPK_MODE)
            {
                p_link = &app_db.br_link[app_hfp_get_active_idx()];
            }
#endif

#if F_APP_LEA_SUPPORT
            app_eq_media_eq_enable(p_link);
#else
            if (p_link)
            {
                app_eq_audio_eq_enable(&p_link->eq_instance, &p_link->audio_eq_enabled);
            }
#endif
        }
        else if (eq_adjust_side ^ app_cfg_const.bud_side)
        {
            uint16_t relay_data_len = eq_len_to_dsp + 1;
            uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

            relay_data[0] = eq_mode;
            memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);

            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                relay_data, relay_data_len);
            free(relay_data);
        }
    }
#if F_APP_APT_SUPPORT
    else if (app_db.sw_eq_type == MIC_SW_EQ)
    {
        if (app_cfg_nv.apt_eq_idx != eq_idx)
        {
            app_cfg_nv.apt_eq_idx = eq_idx;

#if F_APP_ERWS_SUPPORT
            app_relay_async_single(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_EQ_INDEX_SYNC);
#endif
        }

        if (is_play_eq_tone)
        {
            app_eq_play_apt_eq_tone();
        }

        if (eq_adjust_side == BOTH_SIDE_ADJUST)
        {
#if F_APP_ERWS_SUPPORT
            if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                uint16_t relay_data_len = eq_len_to_dsp + 1;
                uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

                relay_data[0] = eq_mode;
                memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);

                app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                   relay_data, relay_data_len, REMOTE_TIMER_HIGH_PRECISION, 0, false);
                free(relay_data);
            }
            else
#endif
            {
                app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);
                eq_enable(app_db.apt_eq_instance);
            }
        }
#if F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
        else if (eq_adjust_side == app_cfg_const.bud_side)
        {
            app_eq_param_set(eq_mode, eq_idx, &cmd_ptr[0], eq_len_to_dsp);
            eq_enable(app_db.apt_eq_instance);
        }
        else if (eq_adjust_side ^ app_cfg_const.bud_side)
        {
            uint16_t relay_data_len = eq_len_to_dsp + 1;
            uint8_t *relay_data = (uint8_t *)malloc(relay_data_len);

            relay_data[0] = eq_mode;
            memcpy(&relay_data[1], &cmd_ptr[0], eq_len_to_dsp);

            app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_EQ_DATA,
                                                relay_data, relay_data_len);
            free(relay_data);
        }
#endif
    }
#endif
    else
    {
        ret = false;
    }

exit:
    return ret;
}

uint8_t app_eq_judge_audio_eq_mode(void)
{
    uint8_t new_eq_mode = NORMAL_MODE;

#if F_APP_ANC_SUPPORT
    bool is_anc_on = app_anc_is_anc_on_state(app_db.current_listening_state);

#if F_APP_SUPPORT_ANC_APT_COEXIST
    is_anc_on |= app_listening_is_anc_apt_on_state(app_db.current_listening_state);
#endif
#endif

    if (0)
    {
    }
#if F_APP_LINEIN_SUPPORT
    else if (app_line_in_plug_state_get() && (eq_utils_num_get(SPK_SW_EQ, LINE_IN_MODE) != 0))
    {
        new_eq_mode = LINE_IN_MODE;
    }
#endif
#if F_APP_VOICE_SPK_EQ_SUPPORT
    else if (app_hfp_sco_is_connected() && (eq_utils_num_get(SPK_SW_EQ, VOICE_SPK_MODE) != 0))
    {
        new_eq_mode = VOICE_SPK_MODE;
    }
#endif
    else if (app_db.gaming_mode && (eq_utils_num_get(SPK_SW_EQ, GAMING_MODE) != 0))
    {
        new_eq_mode = GAMING_MODE;
    }
#if F_APP_ANC_SUPPORT
    else if (is_anc_on && (eq_utils_num_get(SPK_SW_EQ, ANC_MODE) != 0))
    {
        new_eq_mode = ANC_MODE;
    }
#endif
    else
    {
        new_eq_mode = NORMAL_MODE;
    }

    return new_eq_mode;
}

void app_eq_change_audio_eq_mode(bool report_when_eq_mode_change)
{
    uint8_t new_eq_mode;
    uint8_t eq_idx;

    new_eq_mode = app_eq_judge_audio_eq_mode();

    if (app_db.spk_eq_mode != new_eq_mode)
    {
        APP_PRINT_TRACE2("app_eq_change_audio_eq_mode: eq_mode %d -> %d", app_db.spk_eq_mode,
                         new_eq_mode);
        app_db.spk_eq_mode = new_eq_mode;
        app_eq_idx_check_accord_mode();
        eq_idx = app_cfg_nv.eq_idx;

#if F_APP_USER_EQ_SUPPORT
        if (app_eq_is_valid_user_eq_index(SPK_SW_EQ, new_eq_mode, eq_idx))
        {
            app_eq_index_set(SPK_SW_EQ, new_eq_mode, eq_idx);
        }
        else
#endif
        {
            if (!app_db.eq_ctrl_by_src)
            {
                app_eq_index_set(SPK_SW_EQ, new_eq_mode, eq_idx);
            }
        }

#if F_APP_ERWS_SUPPORT
        if (app_cfg_nv.bud_role != REMOTE_SESSION_ROLE_SECONDARY)
#endif
        {
            if (report_when_eq_mode_change)
            {
                app_report_eq_idx(EQ_INDEX_REPORT_BY_CHANGE_MODE);
            }
        }
    }
}

void app_eq_cmd_handle(uint8_t *cmd_ptr, uint16_t cmd_len, uint8_t cmd_path, uint8_t app_idx,
                       uint8_t *ack_pkt)
{
    uint16_t cmd_id = (uint16_t)(cmd_ptr[0] | (cmd_ptr[1] << 8));
    ack_pkt[2] = CMD_SET_STATUS_COMPLETE;

    switch (cmd_id)
    {
    case CMD_AUDIO_EQ_QUERY:
        {
            uint8_t query_type;
            uint8_t buf[11];
            uint8_t eq_reply_len = 0;

            query_type = cmd_ptr[2];
            buf[0] = query_type;

            if (query_type == AUDIO_EQ_QUERY_STATE)
            {
                buf[1] = 1;
                eq_reply_len = 2;
            }
            else if (query_type == AUDIO_EQ_QUERY_NUM)
            {
                buf[1] = eq_utils_num_get(SPK_SW_EQ, NORMAL_MODE);
                buf[2] = eq_utils_num_get(SPK_SW_EQ, GAMING_MODE);

#if F_APP_APT_SUPPORT
                buf[3] = eq_utils_num_get(MIC_SW_EQ, APT_MODE);
#endif

#if F_APP_ANC_SUPPORT
                buf[4] = eq_utils_num_get(SPK_SW_EQ, ANC_MODE);
#endif
                eq_reply_len = 5;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_event(cmd_path, EVENT_AUDIO_EQ_REPLY, app_idx, buf, eq_reply_len);
        }
        break;

    case CMD_AUDIO_EQ_QUERY_PARAM:
        {
            uint8_t eq_mode = cmd_ptr[2];
            uint8_t buf[23] = {0};
            uint32_t capacity;
            uint16_t support_sample_rate = app_eq_get_supported_sample_rate();

            /* EQ state */
            buf[0] = 1;

#if F_APP_USER_EQ_SUPPORT
            /* EQ mapping */
            buf[1] = app_cfg_const.user_eq_spk_eq_num;
            buf[2] = app_cfg_const.user_eq_mic_eq_num;
#endif

            eq_utils_get_capacity(SPK_SW_EQ, NORMAL_MODE, &capacity);
            buf[3] = LO_WORD(capacity);
            buf[4] = HI_WORD(capacity);

            eq_utils_get_capacity(SPK_SW_EQ, GAMING_MODE, &capacity);
            buf[5] = LO_WORD(capacity);
            buf[6] = HI_WORD(capacity);

#if F_APP_APT_SUPPORT
            eq_utils_get_capacity(MIC_SW_EQ, APT_MODE, &capacity);
            buf[7] = LO_WORD(capacity);
            buf[8] = HI_WORD(capacity);
#endif

#if F_APP_ANC_SUPPORT
            eq_utils_get_capacity(SPK_SW_EQ, ANC_MODE, &capacity);
            buf[9] = LO_WORD(capacity);
            buf[10] = HI_WORD(capacity);
#endif

            /* sample rate */
            buf[11] = app_eq_sample_rate_get();
            buf[12] = LO_WORD(support_sample_rate);
            buf[13] = HI_WORD(support_sample_rate);

            /* spk EQ mode & index*/
            buf[14] = app_db.spk_eq_mode;
            buf[15] = app_cfg_nv.eq_idx_normal_mode_record;
            buf[16] = app_cfg_nv.eq_idx_gaming_mode_record;

#if F_APP_ANC_SUPPORT
            buf[17] = app_cfg_nv.eq_idx_anc_mode_record;
#endif

#if F_APP_APT_SUPPORT
            /* mic EQ mode & index */
            buf[18] = APT_MODE;
            buf[19] = app_cfg_nv.apt_eq_idx;
#endif

            eq_utils_get_capacity(SPK_SW_EQ, VOICE_SPK_MODE, &capacity);
            buf[20] = LO_WORD(capacity);
            buf[21] = HI_WORD(capacity);
            buf[22] = 0;

            app_report_event(cmd_path, EVENT_AUDIO_EQ_REPLY_PARAM, app_idx, buf, sizeof(buf));
        }
        break;

    case CMD_AUDIO_EQ_PARAM_SET:
        {
            uint8_t eq_idx = cmd_ptr[2];
            uint8_t eq_adjust_side = cmd_ptr[3];
            uint8_t eq_mode = cmd_ptr[5];
            uint8_t save_eq = cmd_ptr[6];
            uint8_t type = cmd_ptr[7];
            uint8_t seq = cmd_ptr[8];

            bool is_play_eq_tone = false;
            uint16_t eq_len_to_dsp = 0;
            uint32_t sample_rate;
            T_AUDIO_EQ_INFO *p_audio_eq_info = NULL;
            uint8_t *eq_data = NULL;

#if F_APP_VOICE_SPK_EQ_SUPPORT
            if (eq_mode == VOICE_SPK_MODE)
            {
                sample_rate = AUDIO_EQ_SAMPLE_RATE_16KHZ;
            }
            else
#endif
            {
                sample_rate = app_eq_sample_rate_get();
            }

            app_db.sw_eq_type = cmd_ptr[4];

            if ((app_db.sw_eq_type == SPK_SW_EQ) && (app_cfg_nv.eq_idx != eq_idx) ||
                (app_db.sw_eq_type == MIC_SW_EQ) && (app_cfg_nv.apt_eq_idx != eq_idx))
            {
                /* change eq, it needs to play eq tone */
                is_play_eq_tone = true;
            }

            if ((app_db.sw_eq_type == SPK_SW_EQ) && (app_db.spk_eq_mode != eq_mode))
            {
                if (save_eq != SAVE_EQ_FIELD_ONLY_SAVE)
                {
                    ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    break;
                }
            }

            if (cmd_path == CMD_PATH_SPP || cmd_path == CMD_PATH_IAP)
            {
                p_audio_eq_info = &app_db.br_link[app_idx].audio_set_eq_info;
            }
            else if (cmd_path == CMD_PATH_LE)
            {
                p_audio_eq_info = &app_db.le_link[app_idx].audio_set_eq_info;
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            if (type == AUDIO_EQ_FRAME_SINGLE || type == AUDIO_EQ_FRAME_START)
            {
                p_audio_eq_info->eq_seq = EQ_INIT_SEQ;
                p_audio_eq_info->eq_mode = cmd_ptr[5];
                p_audio_eq_info->eq_data_len = (uint16_t)(cmd_ptr[9] | cmd_ptr[10] << 8);
                p_audio_eq_info->eq_data_offset = 0;

                if (type == AUDIO_EQ_FRAME_START)
                {
                    if (p_audio_eq_info->eq_data_buf != NULL)
                    {
                        free(p_audio_eq_info->eq_data_buf);
                    }

                    p_audio_eq_info->eq_data_buf = malloc(p_audio_eq_info->eq_data_len);

                    if (p_audio_eq_info->eq_data_buf == NULL)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        break;
                    }
                }
            }

            if (seq != p_audio_eq_info->eq_seq)
            {
                ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            if (type == AUDIO_EQ_FRAME_SINGLE)
            {
                eq_data = &cmd_ptr[11];
            }
            else
            {
                uint16_t frame_len = 0;
                uint8_t cmd_idx;

                if (type == AUDIO_EQ_FRAME_START)
                {
                    frame_len = (uint16_t)(cmd_ptr[11] | cmd_ptr[12] << 8);
                    cmd_idx = 13;
                }
                else
                {
                    frame_len = (uint16_t)(cmd_ptr[9] | cmd_ptr[10] << 8);
                    cmd_idx = 11;
                }

                p_audio_eq_info->eq_seq = p_audio_eq_info->eq_seq + 1;
                memcpy(p_audio_eq_info->eq_data_buf + p_audio_eq_info->eq_data_offset, &cmd_ptr[cmd_idx],
                       frame_len);
                p_audio_eq_info->eq_data_offset += frame_len;
                eq_data = p_audio_eq_info->eq_data_buf;
            }

            if (type == AUDIO_EQ_FRAME_SINGLE || type == AUDIO_EQ_FRAME_END)
            {
                uint8_t stage_num = app_eq_get_stage_num_accord_eq_len(p_audio_eq_info->eq_data_len);

                eq_len_to_dsp = p_audio_eq_info->eq_data_len - stage_num * EXT_STAGE_NUM_SIZE_VER_2
                                - EQ_INFO_HEADER_SIZE - EQ_SAMPLE_RATE_SIZE;

                if (app_db.sw_eq_type == SPK_SW_EQ)
                {
                    app_eq_sync_idx_accord_eq_mode(eq_mode, eq_idx);
                }

                if ((eq_data[0] == sample_rate) && (save_eq != SAVE_EQ_FIELD_ONLY_SAVE))
                {
                    if (!app_eq_cmd_operate(eq_mode, eq_adjust_side, is_play_eq_tone, eq_idx, eq_len_to_dsp,
                                            &eq_data[1]))
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                    }
                }
                else
                {
                    /* wrong sample rate */
                    if (save_eq == SAVE_EQ_FIELD_ONLY_APPLY)
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }
                }

#if F_APP_USER_EQ_SUPPORT
                if (save_eq != SAVE_EQ_FIELD_ONLY_APPLY)
                {
                    app_eq_save_user_eq_to_ftl((T_EQ_TYPE)app_db.sw_eq_type, eq_mode, eq_idx, eq_data[0],
                                               eq_adjust_side,
                                               &eq_data[1], eq_len_to_dsp,
                                               &eq_data[1 + eq_len_to_dsp],
                                               p_audio_eq_info->eq_data_len - eq_len_to_dsp - 1);
                }
#endif
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_AUDIO_EQ_PARAM_GET:
        {
            if (cmd_len < 5)
            {
                /* if the length of cmd less than 5, the EQ version is 1.1 */
                T_SRC_SUPPORT_VER_FORMAT *version = app_cmd_get_src_version(cmd_path, app_idx);

                version->eq_spec_ver_major = 1;
                version->eq_spec_ver_minor = 1;
                ack_pkt[2] = CMD_SET_STATUS_DISALLOW;
                app_cmd_update_eq_ctrl(false, true);
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            T_AUDIO_EQ_REPORT_DATA eq_report_data;

            if (app_eq_get_link_info(cmd_path, app_idx, &eq_report_data))
            {
                T_AUDIO_EQ_INFO *p_eq_info = eq_report_data.eq_info;

                p_eq_info->eq_idx = cmd_ptr[2];
                p_eq_info->adjust_side = cmd_ptr[3];
                p_eq_info->sw_eq_type = cmd_ptr[4];
                p_eq_info->eq_mode = cmd_ptr[5];
                p_eq_info->eq_seq = EQ_INIT_SEQ;
                p_eq_info->eq_data_offset = 0;

                if ((p_eq_info->adjust_side == app_cfg_const.bud_side) ||
                    (p_eq_info->adjust_side == BOTH_SIDE_ADJUST))
                {
                    if (app_eq_report_get_eq_extend_info(p_eq_info))
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_eq_report_eq_frame(&eq_report_data);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
                else
                {
#if ((F_APP_USER_EQ_SUPPORT && F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT) || (F_APP_AUDIO_VOCIE_SPK_EQ_INDEPENDENT_CFG == 1))
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        uint8_t buf[6] = {0};

                        buf[0] = EQ_SYNC_GET_SECONDARY_EQ_INFO;
                        buf[1] = p_eq_info->sw_eq_type;
                        buf[2] = p_eq_info->eq_idx;
                        buf[3] = p_eq_info->eq_mode;
                        buf[4] = cmd_path;
                        buf[5] = app_idx;

                        app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_USER_EQ,
                                                            buf, sizeof(buf));
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PARAMETER_ERROR;
                    }

                    app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
#endif
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_AUDIO_EQ_INDEX_SET:
        {
            uint8_t eq_idx;
            uint8_t eq_mode;
            bool is_play_eq_tone = false;

            eq_idx = cmd_ptr[2];
            eq_mode = cmd_ptr[3];

            if (app_cfg_nv.eq_idx != eq_idx)
            {
                is_play_eq_tone = true;
            }

            if (app_db.spk_eq_mode != eq_mode)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
            else
            {
                app_eq_sync_idx_accord_eq_mode(eq_mode, eq_idx);
                app_cfg_nv.eq_idx = eq_idx;

#if F_APP_ERWS_SUPPORT
                if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                {
                    app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY,
                                                       APP_REMOTE_MSG_SYNC_DEFAULT_EQ_INDEX, &eq_idx, sizeof(uint8_t), REMOTE_TIMER_HIGH_PRECISION, 0,
                                                       false);
                }
                else
#endif
                {
                    app_eq_index_set(SPK_SW_EQ, eq_mode, eq_idx);
                    T_APP_BR_LINK *p_link = &app_db.br_link[app_a2dp_get_active_idx()];

#if F_APP_VOICE_SPK_EQ_SUPPORT
                    if (eq_mode == VOICE_SPK_MODE)
                    {
                        p_link = &app_db.br_link[app_hfp_get_active_idx()];
                    }
#endif

#if F_APP_LEA_SUPPORT
                    app_eq_media_eq_enable(p_link);
#else
                    if (p_link)
                    {
                        app_eq_audio_eq_enable(&p_link->eq_instance, &p_link->audio_eq_enabled);
                    }
#endif
                }

                if (is_play_eq_tone)
                {
                    app_eq_play_audio_eq_tone();
                }

                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            }
        }
        break;

    case CMD_AUDIO_EQ_INDEX_GET:
        {
            uint8_t eq_mode = cmd_ptr[2];

            if (app_db.spk_eq_mode != eq_mode)
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                break;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_eq_idx(EQ_INDEX_REPORT_BY_GET_EQ_INDEX);
        }
        break;

#if F_APP_APT_SUPPORT
    case CMD_APT_EQ_INDEX_SET:
        {
            uint8_t event_data = cmd_ptr[2];
            uint8_t eq_num;
            bool is_play_eq_tone = false;
            eq_num = eq_utils_num_get(MIC_SW_EQ, APT_MODE);

            if (app_cfg_nv.apt_eq_idx != event_data)
            {
                is_play_eq_tone = true;
            }

            if (eq_num != 0)
            {
                app_cfg_nv.apt_eq_idx = event_data;

                if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_PRIMARY ||
                    app_db.remote_session_state == REMOTE_SESSION_STATE_DISCONNECTED)
                {
                    if (is_play_eq_tone)
                    {
                        app_eq_play_apt_eq_tone();
                    }

#if F_APP_ERWS_SUPPORT
                    if (app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
                    {
                        app_relay_sync_single_with_raw_msg(APP_MODULE_TYPE_APT, APP_REMOTE_MSG_APT_EQ_DEFAULT_INDEX_SYNC,
                                                           &event_data, sizeof(uint8_t), REMOTE_TIMER_HIGH_PRECISION,
                                                           0, false);
                    }
                    else
#endif
                    {
                        app_eq_index_set(MIC_SW_EQ, APT_MODE, app_cfg_nv.apt_eq_idx);
                        eq_enable(app_db.apt_eq_instance);
                    }
                }
            }
            else
            {
                ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
            }

            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
        }
        break;

    case CMD_APT_EQ_INDEX_GET:
        {
            app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
            app_report_apt_eq_idx(EQ_INDEX_REPORT_BY_GET_APT_EQ_INDEX);
        }
        break;
#endif

#if F_APP_USER_EQ_SUPPORT
    case CMD_RESET_EQ_DATA:
        {
            T_AUDIO_EQ_REPORT_DATA eq_report_data;

            uint8_t eq_type = cmd_ptr[2];
            uint8_t eq_mode = cmd_ptr[3];
            uint8_t eq_idx = cmd_ptr[4];
            uint8_t adjust_side = cmd_ptr[5];
            uint8_t buf[4] = {EQ_SYNC_RESET_EQ, eq_type, eq_idx, eq_mode};

#if F_APP_ERWS_SUPPORT
            T_APP_MODULE_TYPE module_type = APP_MODULE_TYPE_AUDIO_POLICY;
            uint8_t msg_type = APP_REMOTE_MSG_SYNC_DEFAULT_EQ_INDEX;

#if F_APP_APT_SUPPORT
            if (eq_type == MIC_SW_EQ)
            {
                module_type = APP_MODULE_TYPE_APT;
                msg_type = APP_REMOTE_MSG_APT_EQ_DEFAULT_INDEX_SYNC;
            }
#endif

            if (adjust_side == BOTH_SIDE_ADJUST &&
                app_db.remote_session_state == REMOTE_SESSION_STATE_CONNECTED)
            {
                app_eq_reset_user_eq((T_EQ_TYPE)eq_type, eq_mode, eq_idx);
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_USER_EQ, buf,
                                                    sizeof(buf));
                app_relay_sync_single_with_raw_msg(module_type, msg_type, &eq_idx, sizeof(uint8_t),
                                                   REMOTE_TIMER_HIGH_PRECISION, 0, false);
            }
#if F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
            else if (adjust_side != app_cfg_const.bud_side && adjust_side != BOTH_SIDE_ADJUST)
            {
                app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_USER_EQ, buf,
                                                    sizeof(buf));
                app_relay_async_single_with_raw_msg(module_type, msg_type, &eq_idx, sizeof(uint8_t));
            }
#endif
            else
#endif  /* F_APP_ERWS_SUPPORT */
            {
                app_eq_reset_user_eq((T_EQ_TYPE) eq_type, eq_mode, eq_idx);
                app_eq_index_set((T_EQ_TYPE) eq_type, eq_mode, eq_idx);
            }

            if (app_eq_get_link_info(cmd_path, app_idx, &eq_report_data))
            {
                T_AUDIO_EQ_INFO *p_eq_info = eq_report_data.eq_info;

                p_eq_info->sw_eq_type = eq_type;
                p_eq_info->eq_mode = eq_mode;
                p_eq_info->eq_idx = eq_idx;
                p_eq_info->adjust_side = adjust_side;
                p_eq_info->eq_seq = EQ_INIT_SEQ;
                p_eq_info->eq_data_offset = 0;

#if F_APP_SEPARATE_ADJUST_APT_EQ_SUPPORT
                if (adjust_side != BOTH_SIDE_ADJUST && adjust_side != app_cfg_const.bud_side)
                {
                    uint8_t buffer[6] = {EQ_SYNC_GET_SECONDARY_EQ_INFO, eq_type, eq_idx, eq_mode, cmd_path, app_idx};

                    app_relay_async_single_with_raw_msg(APP_MODULE_TYPE_AUDIO_POLICY, APP_REMOTE_MSG_SYNC_USER_EQ,
                                                        buffer, sizeof(buffer));
                }
                else
#endif
                {
                    if (app_eq_report_get_eq_extend_info(p_eq_info))
                    {
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                        app_eq_report_eq_frame(&eq_report_data);
                    }
                    else
                    {
                        ack_pkt[2] = CMD_SET_STATUS_PROCESS_FAIL;
                        app_report_event(cmd_path, EVENT_ACK, app_idx, ack_pkt, 3);
                    }
                }
            }
        }
        break;
#endif  /* F_APP_USER_EQ_SUPPORT */

    default:
        break;
    }
}

