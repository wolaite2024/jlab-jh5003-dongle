#ifndef _APP_USB_VOL_CONTROL_H_
#define _APP_USB_VOL_CONTROL_H_

#include "usb_host_detect.h"

#define VOL_GAIN_RESET  0xFF

typedef struct
{
    int16_t uac_spk_vol_gain;
    uint8_t host_type;
    bool is_max_min_vol;
} T_USB_SPK_INFO;

/**
 * app_usb_vol_control.h
 *
 * @brief   volume control command
 *
 */
typedef enum
{
    SPK_VOL_UP    = 0x00,
    SPK_VOL_DOWN  = 0x01,
} T_APP_VOL_CTRL;

typedef enum
{
    CTRL_FROM_HOST     = 0x00,
    CTRL_FROM_HEADSET  = 0x01,
} T_APP_CTRL_DIR;

typedef void (*P_HOST_TYPE_SYNC_CB)(uint8_t type);

extern T_USB_SPK_INFO usb_spk_vol;

/**
 * app_usb_vol_control.h
 *
 * @brief   Set usb uac speaker volume gain to DSP.
 *
 * @param[in] handle    audio track handle.
 * @param[in] gain      usb uac current gain(db).
 * @param[in] dir       vol control from host or headset.
 *
 * @return true or false
 *
 */
bool app_usb_volume_db_set(void *handle, int16_t gain, T_APP_CTRL_DIR dir);

/**
 * app_usb_vol_control.h
 *
 * @brief   When adjusting the volume on the headphone end, handle the local volume.
 *
 * @param[in] type      volume up or down.
 * @param[in] handle    audio track handle.
 *
 * @return true or false
 *
 */
bool app_usb_audio_volume_control_handle(T_APP_VOL_CTRL type, void *handle);

/**
 * app_usb_vol_control.h
 *
 * @brief   When adjusting the volume on the headphone end, send hid cmd to the host.
 *
 * @param[in] type      volume up or down.
 *
 */
void app_usb_audio_volume_hid_ctrl(T_APP_VOL_CTRL type);

/**
 * app_usb_vol_control.h
 *
 * @brief   sync host type callback.
 *
 * @param[in] cb
 *
 */
void app_usb_vol_host_type_sync_register(P_HOST_TYPE_SYNC_CB cb);

/**
 *
 * \brief   Register HID infomation.
 * \param   HID related infomation.
 *
 */
void app_vol_control_hid_info_register(T_USB_HOST_DETECT_HID_INFO info);

/**
 *
 * \brief   Play max min volume tone.
 * @param[in] type volume up or down.
 */
void app_usb_play_vol_max_min_tone(T_APP_VOL_CTRL type);
/**
 * app_usb_audio.h
 *
 * \brief   init app usb vol control
 *
 */
void app_usb_vol_control_init(void);


#endif
