#include <string.h>
#include "trace.h"
#include "le_vcs_service.h"
#include "vocs_mgr.h"
#include "vcs_mgr.h"
#include "aics_mgr.h"
#include "mics_mgr.h"
#include "app_usb_layer.h"
#include "app_timer.h"
#include "app_dsp_cfg.h"

#if UAL_CONSOLE_PRINT
#include "console.h"
#endif
#include "app_audio_path.h"

#include "ual_dev_mgr.h"

#if LE_AUDIO_VCS_SUPPORT
#define LE_AUDIO_VCS_LEFT       0
#define LE_AUDIO_VCS_RIGHT      1

#define LE_AUDIO_VCS_LEFT_DES   "Dongle left channel"
#define LE_AUDIO_VCS_RIGHT_DES  "Dongle right channel"

#define LE_AUDIO_LINE_IN_DES        "Dongle Line in"
#define LE_AUDIO_USB_DS_DES         "Dongle USB DS"
#define LE_AUDIO_MIC_DES            "Dongle MIC"
#define LE_AUDIO_BLUETOOTH_DES      "Dongle Bluetooth"

#define LE_AUDIO_LINE_IN_IDX        0
#define LE_AUDIO_MIC_IDX            1
#define LE_AUDIO_USB_DS_IDX         2
#define LE_AUDIO_BLUETOOTH_IDX      3

#define LE_AUDIO_VCS_MAX        255
#define LE_AUDIO_UAC_MAX        0xFFFF
#define LE_AUDIO_DSP_MAX        15

typedef struct
{
    uint8_t timer_idx_vcs_tk;
    uint8_t timer_idx_vcs_rec;

    bool bluetooth_out_enable;          //usb out
    bool aux_record_enable;                 //record in
    bool aux_track_enable;                //track out

    uint8_t tk_volume;
    uint8_t tk_volume_chg;
    uint8_t rec_volume;
    bool    track_muted;
    bool    track_need_mute;
    int16_t left_volume_offset;
    int16_t right_volume_offset;
} T_VCS_DB;

#define VCS_BLUETOOTH_MAX                     255
#define VCS_USB_NORMALIZATION_VAL             130

static T_VCS_DB le_vcs_db;
static uint8_t le_vcs_timer_id;
#define VCS_TRACK_TIMER        1
#define VCS_RECORD_TIMER        2

#define VCS_CHECK_TIME_MS           100

//extern const uint16_t app_audio_dac_gain_table[];

static void le_vcs_notify_volume_change()
{
    T_VCS_PARAM param;
    if (vcs_get_param(&param))
    {
        uint8_t muted = le_vcs_db.track_muted ? VCS_MUTED : VCS_NOT_MUTED;
        if (param.mute != muted ||
            (param.volume_setting * 15 / VCS_BLUETOOTH_MAX) != le_vcs_db.tk_volume)
        {
            param.mute = muted;
            param.volume_setting = le_vcs_db.tk_volume * 15;
            param.change_counter++;
            vcs_set_param(&param);
        }
    }
}

void le_vcs_set_aux_record_state(bool enable)
{
    uint8_t input_status;
    le_vcs_db.aux_record_enable = enable;
    if (enable)
    {
        input_status = AICS_INPUT_STATUS_ACTIVE;
        aics_set_param(0, AICS_PARAM_INPUT_STATUS, 1, &input_status, false);
    }
    else
    {
        input_status = AICS_INPUT_STATUS_INACTIVE;
        aics_set_param(0, AICS_PARAM_INPUT_STATUS, 1, &input_status, false);
    }
}

void le_vcs_set_aux_track_state(bool enable)
{
    le_vcs_db.aux_track_enable = enable;
}

//If the src comes from usb pipe, app should set call this API to set state
void le_vcs_set_bluetooth_out_state(bool enable)
{
    le_vcs_db.bluetooth_out_enable = enable;
}

void le_vcs_usb_set_out_volume(uint16_t value)
{
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[60];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff, "usb volume track set: 0x%x, %d\r\n", value, value);

    console_write(event_buff, buf_len);
#endif

    le_vcs_db.tk_volume_chg = le_vcs_db.tk_volume;

    if (value == 1)
    {
        le_vcs_db.track_need_mute = true;
    }
    else
    {
        le_vcs_db.track_need_mute = false;
    }
    if (value > 1)
    {
        uint8_t volume_idx = (value * 16) / VCS_USB_NORMALIZATION_VAL;
        le_vcs_db.tk_volume_chg = volume_idx == 0 ? 1 : volume_idx;
        if (value == 2)
        {
            le_vcs_db.tk_volume_chg = 0;
        }
    }

    app_start_timer(&le_vcs_db.timer_idx_vcs_tk, "track",
                    le_vcs_timer_id, VCS_TRACK_TIMER, 0, false,
                    VCS_CHECK_TIME_MS);
}

static void le_vcs_set_out_volume(uint16_t value)
{
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[60];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff, "vcs volume track set: 0x%x, %d\r\n", value, value);

    console_write(event_buff, buf_len);
#endif

    le_vcs_db.tk_volume_chg = value * 15 / VCS_BLUETOOTH_MAX;

    app_start_timer(&le_vcs_db.timer_idx_vcs_tk, "track",
                    le_vcs_timer_id, VCS_TRACK_TIMER, 0, false,
                    VCS_CHECK_TIME_MS);
}

void le_vcs_set_in_volume(uint16_t value)
{
#if UAL_CONSOLE_PRINT
    uint8_t event_buff[60];
    uint16_t buf_len;
    buf_len =  sprintf((char *)event_buff, "volume mic set: 0x%x, %d\r\n", value, value);

    console_write(event_buff, buf_len);
#endif

    app_start_timer(&le_vcs_db.timer_idx_vcs_rec, "record",
                    le_vcs_timer_id, VCS_RECORD_TIMER, 0, false,
                    VCS_CHECK_TIME_MS);
    le_vcs_db.rec_volume = (uint8_t)value;
}

uint16_t le_vcs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_VCS_VOLUME_CP_IND:
        {
            T_VCS_VOLUME_CP_IND *p_volume_state = (T_VCS_VOLUME_CP_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_VCS_VOLUME_CP_IND: conn_handle 0x%x, volume_setting %d, mute %d",
                            p_volume_state->conn_handle, p_volume_state->volume_setting,
                            p_volume_state->mute);
            if (!le_vcs_db.aux_track_enable)
            {
                return BLE_AUDIO_CB_RESULT_REJECT;
            }

            if (p_volume_state->mute == VCS_MUTED && !le_vcs_db.track_muted)
            {
                le_vcs_db.track_need_mute = true;
            }
            else if (p_volume_state->mute == VCS_NOT_MUTED && le_vcs_db.track_muted)
            {
                le_vcs_db.track_need_mute = false;
            }
            le_vcs_set_out_volume(p_volume_state->volume_setting);
        }
        break;

    default:
        break;
    }
    return cb_result;
}

uint16_t le_vocs_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_VOCS_WRITE_OFFSET_STATE_IND:
        {
            T_VOCS_WRITE_OFFSET_STATE_IND *p_volume_offset = (T_VOCS_WRITE_OFFSET_STATE_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_VOCS_WRITE_OFFSET_STATE_IND: conn_handle 0x%x, volume_offset %d, srv_instance_id %d",
                            p_volume_offset->conn_handle, p_volume_offset->volume_offset,
                            p_volume_offset->srv_instance_id);
            //FIX TODO
            cb_result = BLE_AUDIO_CB_RESULT_REJECT;
        }
        break;
    default:
        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
        break;
    }
    return cb_result;
}


uint16_t le_aics_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_AICS_CP_IND:
        {
            T_AICS_CP_IND *p_aics_op = (T_AICS_CP_IND *)buf;
            APP_PRINT_INFO3("LE_AUDIO_MSG_AICS_CP_IND: conn_handle 0x%x, cp_op 0x%x srv_instance_id %d",
                            p_aics_op->conn_handle, p_aics_op->cp_op, p_aics_op->srv_instance_id);
            switch (p_aics_op->cp_op)
            {
            case AICS_CP_SET_GAIN_SETTING:
                {
                    cb_result = BLE_AUDIO_CB_RESULT_REJECT;   //FIX TODO
                }
                break;
            case AICS_CP_UNMUTE:
                {
                }
                break;
            case AICS_CP_MUTE:
                {
                }
                break;

            default:
                cb_result = BLE_AUDIO_CB_RESULT_REJECT;
                break;
            }
        }
        break;

    default:
        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
        break;
    }
    return cb_result;
}

uint16_t le_mics_handle_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    switch (msg)
    {
    case LE_AUDIO_MSG_MICS_WRITE_MUTE_IND:
        {
        }
        break;

    default:
        cb_result = BLE_AUDIO_CB_RESULT_REJECT;
        break;
    }
    return cb_result;
}

static void le_audio_vcs_handle_record_timeout(void)
{
    if (le_vcs_db.aux_record_enable)
    {

    }
}

static void le_audio_vcs_timeout_cb(uint8_t timer_evt, uint16_t param)
{

    //                timer_evt, param);
    switch (timer_evt)
    {
    case VCS_TRACK_TIMER:
        app_stop_timer(&le_vcs_db.timer_idx_vcs_tk);
        break;
    case VCS_RECORD_TIMER:
        le_audio_vcs_handle_record_timeout();
        app_stop_timer(&le_vcs_db.timer_idx_vcs_rec);
        break;
    default:
        break;
    }

}

void le_vcs_audio_set_input_mute(uint8_t iterminal, bool enable)
{
    APP_PRINT_INFO2("le_vc_audio_set_input_mute: iterminal %d, enable 0x%x",
                    iterminal, enable);
    T_AICS_INPUT_STATE input_state;
    T_MICS_PARAM mics_param;

    switch (iterminal)
    {
    case IT_MIC:
        {
            if (aics_get_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATE, (uint8_t *)&input_state))
            {
                input_state.mute = enable ? AICS_MUTED : AICS_NOT_MUTED;
                aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                               (uint8_t *)&input_state, false);
            }
            mics_param.mic_mute = enable ? MICS_MUTED : MICS_NOT_MUTE;
            mics_set_param(&mics_param);
        }
        break;

    case IT_UDEV_IN1:
        break;

    case IT_AUX:
        {
            if (aics_get_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_STATE, (uint8_t *)&input_state))
            {
                input_state.mute = enable ? AICS_MUTED : AICS_NOT_MUTED;
                aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                               (uint8_t *)&input_state, false);
            }
        }
        break;

    default:
        break;
    }
}

void le_vcs_audio_set_input_status(uint8_t iterminal, bool enable)
{
    APP_PRINT_INFO2("le_vc_audio_set_input_status: iterminal %d, enable 0x%x",
                    iterminal, enable);
    uint8_t input_status = enable ? AICS_INPUT_STATUS_ACTIVE : AICS_INPUT_STATUS_INACTIVE;
    T_AICS_INPUT_STATE input_state;
    switch (iterminal)
    {
    case IT_MIC:
        {
            if (enable)
            {
                input_state.gain_setting = 0;
                input_state.gain_mode = AICS_GAIN_MODE_AUTOMATIC_ONLY;    //For now dsp don't support adc adjust
                input_state.mute = AICS_NOT_MUTED;
                aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                               (uint8_t *)&input_state,
                               false);
            }
            aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATUS, 1, &input_status, false);
        }
        break;

    case IT_UDEV_IN1:
        break;

    case IT_AUX:
        {
            if (enable)
            {
                input_state.gain_setting = 0;
                input_state.gain_mode = AICS_GAIN_MODE_AUTOMATIC_ONLY;    //For now dsp don't support adc adjust
                input_state.mute = AICS_NOT_MUTED;
                aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                               (uint8_t *)&input_state,
                               false);
            }

            aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_STATUS, 1, &input_status, false);
        }
        break;

    default:
        break;
    }
}

void le_vcs_audio_set_output_mute(uint8_t oterminal, bool enable)
{
    APP_PRINT_INFO2("le_vc_audio_set_output_mute: oterminal %d, enable 0x%x", oterminal, enable);
    le_vcs_db.track_muted = enable;
    switch (oterminal)
    {
    case OT_AUX:
    case OT_SPK:
        {
            le_vcs_notify_volume_change();
        }
        break;

    default:
        break;
    }
}


void le_vcs_audio_set_output_vol_value(uint8_t oterminal, uint8_t volume)
{
    APP_PRINT_INFO2("le_vc_audio_set_output_vol_value: oterminal %d, volume 0x%x", oterminal, volume);
    le_vcs_db.tk_volume = volume;
    switch (oterminal)
    {
    case OT_AUX:
    case OT_SPK:
        {
            le_vcs_notify_volume_change();
        }
        break;

    default:
        break;
    }
}

void le_vcs_init(void)
{
    uint8_t features[] = {0, 0};
    uint8_t vcs_id_array[] = {LE_AUDIO_LINE_IN_IDX};
    uint8_t mics_id_array[] = {LE_AUDIO_MIC_IDX};
    T_VOCS_PARAM_SET param;
    T_BLE_AUDIO_VC_MIC_PARAMS vc_mic_param;
    memset(&le_vcs_db, 0, sizeof(T_VCS_DB));
    memset(&vc_mic_param, 0, sizeof(T_BLE_AUDIO_VC_MIC_PARAMS));

    /* set dongle default vol to usb init vol */
    le_vcs_db.tk_volume_chg = 15;
    vc_mic_param.vocs_num = 2;
    vc_mic_param.aics_vcs_num = 1;
    vc_mic_param.aics_mics_num = 1;
    vc_mic_param.aics_total_num = 2;
    vc_mic_param.vcs_enable = true;
    vc_mic_param.mics_enable = true;
    vc_mic_param.p_vocs_feature_tbl = features;
    vc_mic_param.p_aics_vcs_tbl = vcs_id_array;
    vc_mic_param.p_aics_mics_tbl = mics_id_array;

    ble_audio_vc_mic_init(&vc_mic_param);

    param.set_mask = VOCS_AUDIO_LOCATION_FLAG | VOCS_AUDIO_OUTPUT_DES_FLAG;
    param.audio_location = AUDIO_LOCATION_FL;
    param.output_des.p_output_des = LE_AUDIO_VCS_LEFT_DES;
    param.output_des.output_des_len = strlen(LE_AUDIO_VCS_LEFT_DES);
    vocs_set_param(LE_AUDIO_VCS_LEFT, &param);

    param.set_mask = VOCS_AUDIO_LOCATION_FLAG | VOCS_AUDIO_OUTPUT_DES_FLAG;
    param.audio_location = AUDIO_LOCATION_FR;
    param.output_des.p_output_des = LE_AUDIO_VCS_RIGHT_DES;
    param.output_des.output_des_len = strlen(LE_AUDIO_VCS_RIGHT_DES);
    vocs_set_param(LE_AUDIO_VCS_RIGHT, &param);

    //FIX TODO it needs to get value from audio
    T_AICS_INPUT_STATE input_state;
    T_AICS_GAIN_SETTING_PROP gain_setting_prop = {5, -127, 127};    //FIX TODO
    T_AUDIO_INPUT_TYPE input_type;
    uint8_t input_status = AICS_INPUT_STATUS_INACTIVE;

    input_state.gain_setting = 0;
    input_state.gain_mode = AICS_GAIN_MODE_AUTOMATIC_ONLY;    //For now dsp don't support adc adjust
    input_state.mute = AICS_NOT_MUTED;
    aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                   (uint8_t *)&input_state,
                   false);
    aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_GAIN_SETTING_PROP, sizeof(T_AICS_GAIN_SETTING_PROP),
                   (uint8_t *)&gain_setting_prop, false);
    input_type = AUDIO_INPUT_ANALOG;
    aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_TYPE, sizeof(T_AUDIO_INPUT_TYPE),
                   (uint8_t *)&input_type, false);
    aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_STATUS, 1, &input_status, false);
    aics_set_param(LE_AUDIO_LINE_IN_IDX, AICS_PARAM_INPUT_DES, strlen(LE_AUDIO_LINE_IN_DES),
                   LE_AUDIO_LINE_IN_DES, false);

    input_status = AICS_INPUT_STATUS_INACTIVE;

    aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATE, sizeof(T_AICS_INPUT_STATE),
                   (uint8_t *)&input_state,
                   false);
    aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_GAIN_SETTING_PROP, sizeof(T_AICS_GAIN_SETTING_PROP),
                   (uint8_t *)&gain_setting_prop, false);
    input_type = AUDIO_INPUT_MICROPHONE;
    aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_TYPE, sizeof(T_AUDIO_INPUT_TYPE),
                   (uint8_t *)&input_type, false);
    aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_STATUS, 1, &input_status, false);
    aics_set_param(LE_AUDIO_MIC_IDX, AICS_PARAM_INPUT_DES, strlen(LE_AUDIO_MIC_DES),
                   LE_AUDIO_MIC_DES, false);

    T_VCS_PARAM vcs_param;
    vcs_param.volume_setting = 210;
    vcs_param.mute = VCS_NOT_MUTED;
    vcs_param.change_counter = 0;
    vcs_param.volume_flags = VCS_USER_SET_VOLUME_SETTING;
    vcs_param.step_size = 15;
    vcs_set_param(&vcs_param);

    app_timer_reg_cb(le_audio_vcs_timeout_cb, &le_vcs_timer_id);

}
#endif
