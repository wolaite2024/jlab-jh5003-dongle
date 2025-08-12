#if F_APP_USB_HID_SUPPORT
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "trace.h"
#include "usb_hid_desc.h"
#include "usb_hid.h"
#include "errno.h"
#include "app_mmi.h"
#include "app_usb_audio.h"
#include "app_usb_vol_control.h"

#define BIT_POS_VOL_UP        0
#define BIT_POS_VOL_DOWN      1
#define BIT_POS_PLAY_PAUSE    2
#define BIT_POS_NEXT_TRACK    3
#define BIT_POS_PREV_TRACK    4

#if F_APP_GAMING_CONTROLLER_SUPPORT
//--------------------------------------------------------------------------------
// Generic Desktop Page inputReport (Device --> Host)
//--------------------------------------------------------------------------------

typedef struct
{
    uint8_t id;
    uint8_t X;
    uint8_t Y;
    uint8_t Z;
    uint8_t Rx;
    uint8_t Ry;
    uint8_t Rz;

    uint8_t vnd_data;
    uint8_t hat_switch : 4;
    uint8_t button_1 : 1;
    uint8_t button_2 : 1;
    uint8_t button_3 : 1;
    uint8_t button_4 : 1;

    uint8_t button_5 : 1;
    uint8_t button_6 : 1;
    uint8_t button_7 : 1;
    uint8_t button_8 : 1;
    uint8_t button_9 : 1;
    uint8_t button_10 : 1;
    uint8_t button_11 : 1;
    uint8_t button_12 : 1;

    uint8_t button_13 : 1;
    uint8_t button_14 : 1;
    uint8_t button_15 : 1;
    uint8_t resv : 5;
} GAME_PAD_HID_INPUT_REPORT;
#endif

static void *hid_mmi_handle = NULL;

int32_t app_usb_mmi_handle_action(uint8_t action)
{
    uint8_t bit_pos = 0;
    uint8_t report[2] = {HID_REPORT_ID_AUDIO_CONTROL, 0};
    int32_t ret = ESUCCESS;

    APP_PRINT_INFO1("app_hid_ac_handle_mmi, action:0x%x", action);
    switch (action)
    {
    case MMI_DEV_SPK_VOL_UP:
        {
            bit_pos = BIT_POS_VOL_UP;
        }
        break;

    case MMI_DEV_SPK_VOL_DOWN:
        {
            bit_pos = BIT_POS_VOL_DOWN;
        }
        break;

    case MMI_AV_PLAY_PAUSE:
        {
            bit_pos = BIT_POS_PLAY_PAUSE;
        }
        break;

    case MMI_AV_FWD:
        {
            bit_pos = BIT_POS_NEXT_TRACK;
        }
        break;

    case MMI_AV_BWD:
        {
            bit_pos = BIT_POS_PREV_TRACK;
        }
        break;

    default:
        {
            ret = -ESRCH;
            return ret;
        }
    }

    if (hid_mmi_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .congestion_ctrl = HID_CONGESTION_CTRL_DROP_CUR,
            .rsv = 0,
            .mtu = CONSUMER_CTRL_MAX_TRANSMISSION_UNIT
        };
        hid_mmi_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, CONSUMER_CTRL_MAX_PENDING_REQ_NUM,
                                                NULL);
    }

#if F_APP_GAMING_DONGLE_TRANS_UAC_VOL_TO_HEADSET
    if ((action == MMI_DEV_SPK_VOL_UP) || (action == MMI_DEV_SPK_VOL_DOWN))
    {
        T_APP_VOL_CTRL type = SPK_VOL_UP;

        if (action == MMI_DEV_SPK_VOL_DOWN)
        {
            type = SPK_VOL_DOWN;
        }

        app_usb_audio_volume_hid_ctrl(type);
        app_usb_play_vol_max_min_tone(type);
#if F_APP_USB_AUDIO_SUPPORT
        app_usb_audio_local_gain_change(type);
#endif
    }
    else
#endif
    {
        report[1] = 1 << bit_pos;
        usb_hid_data_pipe_send(hid_mmi_handle, report, 2);

        report[1] = 0;
        usb_hid_data_pipe_send(hid_mmi_handle, report, 2);
    }

    return ret;
}

#if F_APP_GAMING_CONTROLLER_SUPPORT
void app_usb_mmi_game_pad_test(void)
{
    static uint8_t s_cnt = 0;
    s_cnt++;

    if (hid_mmi_handle == NULL)
    {
        T_USB_HID_ATTR attr =
        {
            .zlp = 1,
            .high_throughput = 0,
            .rsv = 0,
            .mtu = HID_MAX_TRANSMISSION_UNIT
        };
        hid_mmi_handle = usb_hid_data_pipe_open(HID_INT_IN_EP_1, attr, HID_MAX_PENDING_REQ_NUM,
                                                NULL);
    }

    GAME_PAD_HID_INPUT_REPORT gd_data;
    memset(&gd_data, 0, sizeof(GAME_PAD_HID_INPUT_REPORT));
    gd_data.id = REPORT_ID_GAME_PAD_INPUT;
    if (s_cnt == 1)
    {
        gd_data.X = 0x82;
        gd_data.Y = 0x80;
        gd_data.Z = 0x7F;
        gd_data.Rz = 0x7F;
        gd_data.hat_switch = 8;
        gd_data.button_1 = 1;
        usb_hid_data_pipe_send(hid_mmi_handle, (uint8_t *)&gd_data, sizeof(gd_data));
    }
    else if (s_cnt == 2)
    {
        gd_data.X = 0x70;
        gd_data.Y = 0x60;
        gd_data.Z = 0x7;
        gd_data.Rz = 0x7F;
        gd_data.Rx = 0x02;
        gd_data.Ry = 0x08;
        gd_data.hat_switch = 6;
        gd_data.button_3 = 1;
        gd_data.button_5 = 1;
        usb_hid_data_pipe_send(hid_mmi_handle, (uint8_t *)&gd_data, sizeof(gd_data));
    }
    else
    {
        gd_data.X = 0x0;
        gd_data.Y = 0x0;
        gd_data.Z = 0x88;
        gd_data.Rz = 0x72;
        gd_data.Rx = 0x72;
        gd_data.Ry = 0x78;
        usb_hid_data_pipe_send(hid_mmi_handle, (uint8_t *)&gd_data, sizeof(gd_data));
        s_cnt = 0;
    }

}
#endif

void app_usb_mmi_init(void)
{

}
#endif

