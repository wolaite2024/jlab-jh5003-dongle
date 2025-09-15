#include "stdint.h"
#include "app_msg.h"
#include "stdbool.h"
#include "app_hid_report_desc.h"
#ifndef APP_USB_HID_H
#define APP_USB_HID_H


#define APP_USB_HID_MAX_IN_SIZE 63

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

#define USB_HID_MSG_TYPE_GAIA       REPORT_ID_GAIA_FEATURE

#define USB_HID_MSG_TYPE_HID_SET_REPORT             0xF0
#define USB_HID_MSG_TYPE_HID_SUSPEND_RESUME         0xF1
#define USB_HID_MSG_TYPE_ASP                        0x9a
#define USB_HID_MSG_TYPE_CONSUMER_CRTL              REPORT_ID_CONSUMER_HOT_KEY_INPUT
#define USB_HID_MSG_TYPE_HID_IN_REQUEST             0xF2
#define USB_HID_MSG_TYPE_HID_IN_COMPLETE            0xF3
#define USB_HID_MSG_TYPE_CTRL_DATA_REQUEST          REPORT_ID_CTRL_DATA_OUT_REQUEST
#define USB_HID_MSG_TYPE_HID_BUFFERED_REPORT        0xFA

#if 0
#define TELEPHONY_SEND_BIT_CALL_ANSWER              0x0001
#define TELEPHONY_SEND_BIT_MUTE                     0x0008
#define TELEPHONY_SEND_BIT_HOLD                     0x0010
#define TELEPHONY_SEND_BIT_REJECT_TEAMS             0x0800

#define TELEPHONY_RECV_MASK_CALL_ANSWER             0x0001
#define TELEPHONY_RECV_MASK_MUTE                    0x0004
#define TELEPHONY_RECV_MASK_RING                    0x0008
#define TELEPHONY_RECV_MASK_HOLD                    0x0010
#endif
typedef union
{
    uint8_t data[2];
    struct
    {
        uint8_t offHook      : 1;
        uint8_t speaker      : 1;
        uint8_t mute         : 1;
        uint8_t ring         : 1;
        uint8_t hold         : 1;
        uint8_t mic          : 1;
        uint8_t online       : 1;
        uint8_t reserved     : 1;
    } report;
} APP_TELEPHONY_HID_OUTPUT_REPORT;

typedef union
{
    uint8_t data[2];
    struct
    {
        uint8_t hook_switch   : 1;
        uint8_t busy_tone     : 1;
        uint8_t line          : 1;
        uint8_t mute          : 1;
        uint8_t flash         : 1;
        uint8_t redial        : 1;
        uint8_t speed_dial    : 1;
        uint8_t key0          : 1;

        uint8_t key1          : 1;
        uint8_t key2          : 1;
        uint8_t key3          : 1;
        uint8_t reject        : 1;
        uint8_t button0       : 1;
        uint8_t button1       : 1;
        uint8_t button2       : 1;
        uint8_t button3       : 1;
    } report;
} APP_TELEPHONY_HID_INPUT_REPORT;

#define HID_MAX_TRANSMISSION_UNIT                   0x40
#define HID_MAX_PENDING_REQ_NUM                     0x08

typedef struct _T_APP_USB_HID_CB_F
{
    void (*app_usb_hid_bt_ctrl_data_recv_cb)(uint16_t length, uint8_t *data);
    void (*app_usb_hid_telephony_data_recv_cb)(uint16_t code); //see define TELEPHONY_RECV*
} APP_USB_HID_CB_F;

enum CONSUMER_CTRL_KEY_CODE
{
    KEY_RELEASE     =   0x00,
    KEY_VOL_UP      =   0x01,
    KEY_VOL_DOWN    = (0x01 << 1),
    KEY_PLAY_PAUSE  = (0x01 << 2),
    KEY_NEXT_TK     = (0x01 << 3),
    KEY_PREV_TK     = (0x01 << 4),
    KEY_STOP        = (0x01 << 5),
    KEY_FORWARD     = (0x01 << 6),
    KEY_REWIND      = (0x01 << 7),
};

typedef enum
{
    HID_IF_KEYBOARD,
    HID_IF_CONSUMER,
    HID_IF_MOUSE,
    HID_IF_HIDSYSCTRL,
    HID_IF_TELEPHONY,
    HID_IF_ASP,
    HID_IF_GATT,
    HID_IF_GAIA,
    HID_IF_CTRL,
    HID_IF_TEST
} SEND_TYPE;

typedef struct t_usb_hid_db
{
    void     *p_sema;
    uint8_t  *p_get_report_data;
    uint8_t  get_report_len;
    uint16_t telephony_output;
} T_USB_HID_DB;

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

extern APP_USB_HID_CB_F app_usb_hid_cb_f;

bool app_usb_hid_get_report_data_is_ready(uint8_t *data, uint8_t length);
void app_usb_hid_send_report(SEND_TYPE type, uint8_t id, uint8_t *data, uint8_t length,
                             uint32_t pnpinfo);
void app_usb_hid_handle_msg(T_IO_MSG *msg);
bool app_usb_hid_interrupt_in(uint8_t *data, uint8_t data_size);
extern int8_t app_src_os_spk_is_mute;

void app_usb_hid_init(void);
void app_usb_hid_register_cbs(APP_USB_HID_CB_F *functions);
bool app_usb_hid_send_bt_ctrl_data(uint16_t length, uint8_t *buf);
bool app_usb_hid_send_consumer_ctrl_key_down(enum CONSUMER_CTRL_KEY_CODE
                                             key);//key see CONSUMER_CTRL_KEY_CODE
bool app_usb_hid_send_consumer_ctrl_all_key_release(void);
bool app_usb_hid_send_telephony_ctrl_code(uint16_t *code); //code see define TELEPHONY_SEND*
bool app_usb_hid_send_telephony_mute_ctrl(bool mute);
uint16_t app_usb_hid_get_telephony_output(void);
#endif
