#include <bt_types.h>
#include <string.h>
#include <stdlib.h>
#include "trace.h"
//#include "os_sched.h"
#include "os_queue.h"
#include "rtl876x.h"
#include "app_usb.h"
#include "app_usb_hid.h"
#include "app_usb_uac.h"
#include "app_cfg.h"
#include "usb_dev.h"
#include "usb_msg.h"
#include "app_io_msg.h"
#include "section.h"
#include "pm.h"
#ifdef LEGACY_BT_GENERAL
#include "app_general_policy.h"
#endif

#if TARGET_RTL8763EAU
#include "app_device.h"
#include "app_mmi.h"
#include "btm.h"
#include "general_audio_wrapper.h"
#include "legacy_audio_wrapper.h"
#include "le_audio_wrapper.h"
#include "app_led_ctl.h"
#include "os_sync.h"
#if TARGET_LEGACY_GAMING_DONGLE || TARGET_LE_AUDIO_GAMING_DONGLE
#include "app_src_policy.h"
#endif
#endif

#if (UAC_SILENCE_DETECT_SUPPORT == 1)
#include "uac_silence_detect.h"
#endif
#include "hal_adp.h"
#include "usb_audio_stream.h"
#include "app_usb_uac.h"
#include "app_timer.h"

#define DONGLE_BC12_EN  0

#if F_APP_USB_GIP_SUPPORT
#include "usb_gip_driver.h"
#include "usb_composite_dev.h"
#include "usb_dev_driver.h"
#endif

#if F_APP_GAMING_CONTROLLER_SUPPORT
#include "app_usb_controller.h"
#endif

#include "app_usb_audio_wrapper.h"

#if F_APP_LEA_DONGLE_BINDING
#include "app_le_audio.h"
#endif
#if F_APP_USB_HID_SUPPORT && F_APP_USB_AUDIO_SUPPORT
#include "app_ctrl_pkt_policy.h"
#endif

#include "app_downstream_encode.h"

#include "app_upstream_decode.h"
#include "usb_host_detect.h"
#include "app_ipc.h"
#include "app_usb_vol_control.h"

#define USB_MANU_STR_MAX            (20)
#define USB_PRODUCT_STR_MAX         (70)
#define USB_UAC_1st_STR_MAX            (70)
#define USB_UAC_2nd_STR_MAX            (70)

#if F_APP_USB_SUPPORT
#if F_APP_USB_GIP_SUPPORT
static uint8_t usb_gip_flag = 0;
void (*usb_enum_done_cb)(void) = NULL;
void (*usb_start_enum_cb)(void) = NULL;
void (*usb_suspend_cb)(void) = NULL;
#endif

const char *usb_uac_1st_str = "RTL8763EAU - Chat";
const char *usb_uac_2nd_str = "RTL8763EAU - Game";

#if USBLIB_LEGACY

#else

static T_USB_POWER_STATE s_usb_pwr_state = USB_PDN;

typedef enum
{
    USB_EVT_PLUG,
    USB_EVT_UNPLUG,
    USB_EVT_PWR_STATUS_CHG,
} T_USB_EVT;

typedef enum
{
    USB_SRC_STREAMING_DETECT,
} T_APP_USB_TIMER;

typedef struct _app_usb_db
{
    T_USB_POWER_STATE usb_pwr_state;
    bool usb_suspend;
    uint8_t app_usb_config_count;
} T_APP_USB_DB;

static T_APP_USB_DB app_usb_db;

static uint8_t app_usb_timer_queue_id = 0;
static uint8_t timer_idx_usb_src_streaming_detect;

static void app_usb_timer_cback(uint8_t timer_id, uint16_t timer_chann)
{
    switch (timer_id)
    {
    case USB_SRC_STREAMING_DETECT:
        {
            app_stop_timer(&timer_idx_usb_src_streaming_detect);

            app_usb_set_usb_src_streaming(false);
        }
        break;

    default:
        break;
    }
}

#if F_APP_USB_GIP_SUPPORT
void app_usb_gip_cfg_desc(uint16_t idProduct, uint16_t idVendor, char *serialNum)
{
    USB_PRINT_TRACE3("app_usb_gip_cfg_desc, idProduct = %d, idVendor = %d, serialNum= =%s",
                     idProduct, idVendor, serialNum);
    T_USB_DEV_DESC_CFG dev_desc_cfg;
    char *p_manuf_str = NULL;
    char *p_product_str = NULL;
    char *p_serialnum_str = NULL;
    extern const T_STRING_TAB *const dev_stringtabs[];
    uint16_t language = (*dev_stringtabs)->language;

    memset(&dev_desc_cfg, 0, sizeof(T_USB_DEV_DESC_CFG));
    dev_desc_cfg.bDeviceClass  = 0xFF;
    dev_desc_cfg.bDeviceSubClass  = 0x47;
    dev_desc_cfg.bDeviceProtocol   = 0xd0;
    dev_desc_cfg.bcdDevice  = 0x0176;
    dev_desc_cfg.idProduct  = idProduct;
    dev_desc_cfg.idVendor   = idVendor;
    usb_dev_desc_rcfg(dev_desc_cfg);

    p_manuf_str = "Microsoft";
    p_product_str = "Controller";
    p_serialnum_str = serialNum;

    usb_composite_dev_string_remove(language, STRING_ID_MANUFACTURER, p_manuf_str);
    usb_composite_dev_string_remove(language, STRING_ID_PRODUCT, p_product_str);
    usb_composite_dev_string_remove(language, STRING_ID_SERIALNUM, p_serialnum_str);

    usb_composite_dev_string_add(language, STRING_ID_MANUFACTURER, p_manuf_str);
    usb_composite_dev_string_add(language, STRING_ID_PRODUCT, p_product_str);
    usb_composite_dev_string_add(language, STRING_ID_SERIALNUM, p_serialnum_str);
}
#endif

static void app_usb_cfg_device_desc(void)
{
    T_USB_DEV_DESC_CFG dev_desc_cfg;
    memset(&dev_desc_cfg, 0, sizeof(T_USB_DEV_DESC_CFG));
    dev_desc_cfg.bcdDevice  = 0x200;
    dev_desc_cfg.idProduct  = app_cfg_const.dongle_usb_pid;
    dev_desc_cfg.idVendor   = app_cfg_const.dongle_usb_vid;
    usb_dev_desc_rcfg(dev_desc_cfg);
}

static void app_usb_cfg_string(void)
{
    char *p_manuf_str = NULL;
    char *p_product_str = NULL;
    uint8_t str_len;

    str_len = strlen((char *)app_cfg_const.dongle_usb_manuf_string);
    if (str_len > USB_MANU_STR_MAX - 1)
    {
        str_len = USB_MANU_STR_MAX;
        p_manuf_str = malloc(str_len + 1);
        if (p_manuf_str == NULL)
        {
            goto Error_handle;
        }
        memset(p_manuf_str, 0, str_len + 1); //end of manu must be /0
        memcpy(p_manuf_str, app_cfg_const.dongle_usb_manuf_string, str_len);
        usb_dev_desc_string_rcfg(STRING_IDX_MANUFACTURER, p_manuf_str);
    }
    else if (str_len > 0)
    {
        usb_dev_desc_string_rcfg(STRING_IDX_MANUFACTURER, (char *)app_cfg_const.dongle_usb_manuf_string);
    }

    str_len = strlen((char *)app_cfg_const.dongle_usb_product_string);
    if (str_len > USB_PRODUCT_STR_MAX - 1)
    {
        str_len = USB_PRODUCT_STR_MAX;
        p_product_str = malloc(str_len + 1);
        if (p_product_str == NULL)
        {
            goto Error_handle;
        }

        memset(p_product_str, 0, str_len + 1); //end of manu must be /0
        memcpy(p_product_str, app_cfg_const.dongle_usb_product_string, str_len);
        usb_dev_desc_string_rcfg(STRING_IDX_PRODUCT, p_product_str);
    }
    else if (str_len > 0)
    {
        usb_dev_desc_string_rcfg(STRING_IDX_PRODUCT, (char *)app_cfg_const.dongle_usb_product_string);
    }

    str_len = strlen((char *)usb_uac_1st_str);
    if (str_len)
    {
        usb_dev_desc_string_rcfg(STRING_IDX_UAC_1st, (char *)usb_uac_1st_str);
    }

#ifdef ENABLE_UAC2

    str_len = strlen((char *)usb_uac_2nd_str);
    if (str_len)
    {
        usb_dev_desc_string_rcfg(STRING_IDX_UAC_2st, (char *)usb_uac_2nd_str);
    }
#endif
Error_handle:
    if (p_manuf_str)
    {
        free(p_manuf_str);
    }

    if (p_product_str)
    {
        free(p_product_str);
    }
}

#if TARGET_RTL8763EAU
static void app_usb_suspend_clk(void)
{
    uint32_t actual_mhz, actual_mhz_dsp = 0;
    uint32_t pre_cpu_clk = 0;
    uint32_t flag = os_lock();
    if ((s_usb_pwr_state != USB_SUSPENDED) && (s_usb_pwr_state != USB_PDN))
    {
        APP_PRINT_TRACE1("app_usb_suspend_clk s_usb_pwr_state = %d", s_usb_pwr_state);
        os_unlock(flag);
        return;
    }

    pm_cpu_set_auto_slow_enable(true);
    app_usb_db.usb_suspend = true;
    pm_dvfs_set_by_clock();
    pre_cpu_clk = pm_cpu_freq_get();
    pm_cpu_freq_set(40, &actual_mhz);
    pm_dsp1_freq_set(40, &actual_mhz_dsp);
    APP_PRINT_TRACE2("app_usb_suspend_clk, pre_cpu_clk:%d, actual_mhz:%d", pre_cpu_clk, actual_mhz);
    os_unlock(flag);
}

static void app_usb_resume_clk(void)
{
    uint32_t actual_mhz, actual_mhz_dsp = 0;
    uint32_t pre_cpu_clk = 0;

    pm_cpu_set_auto_slow_enable(false);
    app_usb_db.usb_suspend = false;
    pre_cpu_clk = pm_cpu_freq_get();
    pm_cpu_freq_set(100, &actual_mhz);
    pm_dsp1_freq_set(160, &actual_mhz_dsp);
    APP_PRINT_TRACE2("app_usb_resume_clk, pre_cpu_clk:%d, actual_mhz:%d", pre_cpu_clk, actual_mhz);
}
#endif

RAM_TEXT_SECTION
static bool app_usb_dev_trigger_evt(T_USB_EVT evt,  uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_DEV, evt);
    gpio_msg.u.param = param;

    return app_io_msg_send(&gpio_msg);
}

RAM_TEXT_SECTION
bool app_usb_other_trigger_evt(T_USB_OTHER_MODE_MSG evt,  uint32_t param)
{
    T_IO_MSG gpio_msg;

    gpio_msg.type = IO_MSG_TYPE_USB;
    gpio_msg.subtype = USB_MSG(USB_MSG_GROUP_IF_OTHER, evt);
    gpio_msg.u.param = param;

    return app_io_msg_send(&gpio_msg);
}

static void app_usb_adp_state_change_cb(T_ADP_PLUG_EVENT event, void *user_data)
{
    if (event == ADP_EVENT_PLUG_IN)
    {
        app_usb_dev_trigger_evt(USB_EVT_PLUG, 0);
    }
    else if (event == ADP_EVENT_PLUG_OUT)
    {
        app_usb_dev_trigger_evt(USB_EVT_UNPLUG, 0);
    }

}

static bool app_usb_dm_cb(T_USB_DM_EVT evt, T_USB_DM_EVT_PARAM *param)
{
    if (evt == USB_DM_EVT_STATUS_IND)
    {
        T_USB_DM_EVT_PARAM_STATUS_IND *status_ind = (T_USB_DM_EVT_PARAM_STATUS_IND *)param;
        T_USB_POWER_STATE usb_pwr_state = status_ind->state;
        if ((s_usb_pwr_state == USB_SUSPENDED) && \
            ((usb_pwr_state == USB_DEFAULT) ||
             (usb_pwr_state == USB_ADDRESSED) ||
             (usb_pwr_state == USB_CONFIGURED) ||
             (usb_pwr_state == USB_POWERED)))
        {
#if TARGET_RTL8763EAU
            //resume interrupt handle to do
            if (app_device_is_power_on() == true)
            {
                app_usb_resume_clk();
            }
#endif
        }
        s_usb_pwr_state = usb_pwr_state;

        app_usb_dev_trigger_evt(USB_EVT_PWR_STATUS_CHG, usb_pwr_state);
    }
    return true;
}

#if F_APP_USB_GIP_SUPPORT
static bool app_usb_gip_cb(T_USB_GIP_DRV_EVT evt, uint32_t param)
{
    if (evt == USB_DM_EVT_GIP_MICRO_OS_DESC)
    {
    }
    return true;
}
#endif

bool app_usb_is_suspend(void)
{
    return (app_usb_db.usb_pwr_state == USB_SUSPENDED);
}

#if TARGET_RTL8763EAU
static void app_usb_suspend_src(void)
{
    APP_PRINT_INFO1("app_usb_suspend_src mode %x", app_get_cur_bt_mode());

    /* release all audio pipe */
    app_usb_ds_pipe_release();
    upstream_pipe_release();

    bt_mode_change_app_stop_timer();
    if (app_get_cur_bt_mode() == DONGLE_BT_MODE_LEGACY)
    {
#ifdef LEGACY_BT_GAMING
        legacy_audio_exit();
#endif

#ifdef LEGACY_BT_GENERAL
        general_audio_exit();
#endif
    }
    else if (app_get_cur_bt_mode() == DONGLE_BT_MODE_BIS)
    {
        le_audio_bis_exit();
    }
    else
    {
        le_audio_cis_exit();
    }

    bt_device_mode_set(BT_DEVICE_MODE_IDLE);
    app_led_set_mode(APP_LED_MODE_ALL_OFF);
    app_cfg_const.led_support = 0;
}

static void app_usb_resume_src(void)
{
    APP_PRINT_INFO1("app_usb_resume_src mode %x", app_get_cur_bt_mode());
    app_cfg_const.led_support = 1;

    if (app_get_cur_bt_mode() == DONGLE_BT_MODE_LEGACY)
    {
#ifdef LEGACY_BT_GAMING
        legacy_audio_enter();
#endif

#ifdef LEGACY_BT_GENERAL
        general_audio_enter();
#endif
    }
    else if (app_get_cur_bt_mode() == DONGLE_BT_MODE_BIS)
    {
        le_audio_bis_enter();
    }
    else
    {
        le_audio_cis_enter();
    }

    if (app_get_cur_bt_mode() == DONGLE_BT_MODE_LEGACY)
    {
        app_led_set_mode(APP_LED_MODE_BT_LEGACY);
#ifdef LEGACY_BT_GAMING
        if (app_cfg_const.enable_dongle_dual_mode)
        {
#if F_APP_LEGACY_DONGLE_BINDING
#else
            bt_device_mode_set(BT_DEVICE_MODE_CONNECTABLE);
#endif
        }
        else
        {
            legacy_audio_mmi(MMI_BT_LEGACY_LINK1_RECONNECT, NULL);
        }
#endif
#ifdef LEGACY_BT_GENERAL
        general_audio_mmi(MMI_BT_LEGACY_LINK1_RECONNECT, NULL);
#endif
    }
    else if (app_get_cur_bt_mode() == DONGLE_BT_MODE_BIS)
    {
        app_led_set_mode(APP_LED_MODE_BT_BIS);
        le_audio_mmi_bis(MMI_BT_BIS_START, NULL);
    }
    else
    {
        app_led_set_mode(APP_LED_MODE_BT_CIS);

#if F_APP_LEA_DONGLE_BINDING
        app_le_audio_start_auto_pair();
#else
        le_audio_mmi_cis(MMI_BT_CIS_RECONNECT, NULL);
#endif
    }
}

void app_usb_dm_evt_suspend_handle(void)
{
    if (app_device_is_power_on() == true)
    {
        app_usb_suspend_src();
        app_usb_suspend_clk();
    }
}

static void app_usb_dm_evt_resume_handle(void)
{
    if (app_device_is_power_on() == true)
    {
        app_usb_resume_src();
    }
}
#endif

#if F_APP_USB_GIP_SUPPORT
void app_usb_enum_done_register(GIP_CB_ENUM_DONE cb)
{
    usb_enum_done_cb = cb;
}

void app_usb_start_enum_register(GIP_CB_START_ENUM cb)
{
    usb_start_enum_cb = cb;
}

void app_usb_suspend_register(GIP_CB_SUSPEND cb)
{
    usb_suspend_cb = cb;
}
#endif

int app_usb_start(void)
{
#if DONGLE_BC12_EN
    usb_dm_start(true);
#else
    usb_dm_start(false);
#endif
    return true;
}

int app_usb_stop(void)
{
    usb_dm_stop();
    return true;
}

static void app_usb_dm_evt_handle(uint8_t evt, uint32_t data)
{
    T_USB_POWER_STATE usb_pwr_state = (T_USB_POWER_STATE)data;

    APP_PRINT_INFO2("app_usb_dm_evt_handle evt 0x%x data 0x%x", evt, data);
    switch (evt)
    {
    case USB_EVT_PWR_STATUS_CHG:
        {
            APP_PRINT_INFO2("app_usb_dev_trigger_evt, old:0x%x, usb_pwr_state:0x%x", app_usb_db.usb_pwr_state,
                            usb_pwr_state);
            switch (usb_pwr_state)
            {
            case USB_PDN:
                {
                }
                break;
            case USB_SUSPENDED:
                {
#if TARGET_RTL8763EAU
                    app_usb_dm_evt_suspend_handle();
#endif
#if F_APP_USB_GIP_SUPPORT
                    if (usb_suspend_cb)
                    {
                        usb_suspend_cb();
                    }
#endif
                }
                break;
            case USB_POWERED:
            case USB_DEFAULT:
                {
                    if (app_usb_db.usb_pwr_state == USB_SUSPENDED)
                    {
#if TARGET_RTL8763EAU
                        app_usb_dm_evt_resume_handle();
#endif
                    }
                }
                break;
            case USB_ADDRESSED:
                {
                    if (app_usb_db.usb_pwr_state == USB_SUSPENDED)
                    {
#if TARGET_RTL8763EAU
                        app_usb_dm_evt_resume_handle();
#endif
                    }
                }
                break;
            case USB_CONFIGURED:
                {
#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
                    app_usb_audio_set_feedback_state(USB_FEEDBACK_DETECT);
#endif

                    if (app_usb_db.usb_pwr_state == USB_SUSPENDED)
                    {
#if TARGET_RTL8763EAU
                        app_usb_dm_evt_resume_handle();
#endif
                    }

#if F_APP_USB_GIP_SUPPORT
                    if (usb_enum_done_cb)
                    {
                        usb_enum_done_cb();
                    }
#endif
                }
                break;

            default:
                break;
            }
            app_usb_db.usb_pwr_state = usb_pwr_state;
        }
        break;
    case USB_EVT_PLUG:
        {
#if F_APP_USB_GIP_SUPPORT
            if (usb_start_enum_cb)
            {
                usb_start_enum_cb();
            }
            else
#endif
            {
                app_usb_start();
            }
        }
        break;

    case USB_EVT_UNPLUG:
        {
            app_usb_stop();
        }
        break;

    default:
        break;
    }
}

T_USB_POWER_STATE app_usb_power_state(void)
{
    return app_usb_db.usb_pwr_state;
}

#if F_APP_USB_HID_SUPPORT && F_APP_USB_AUDIO_SUPPORT
static int app_usb_host_detect_cb(T_OS_TYPE type)
{
    dongle_status.usb_host_type = type;
    app_gaming_sync_dongle_status();

    return 0;
}
#endif

static void app_usb_other_evt_handle(uint8_t evt, uint32_t data)
{
    APP_PRINT_INFO2("app_usb_other_evt_handle evt 0x%x data 0x%x", evt, data);
    switch (evt)
    {
    case USB_OTHER_EVT_USB_SRC_STREAMING:
        {
            app_usb_set_usb_src_streaming(true);

            app_start_timer(&timer_idx_usb_src_streaming_detect, "usb_src_streaming_detect",
                            app_usb_timer_queue_id, USB_SRC_STREAMING_DETECT, 0, false,
                            USB_SRC_STREAMING_DETECT_TIMEOUT);
        }
        break;

#if F_APP_USB_AUDIO_FEEDBACK_SUPPORT
    case USB_OTHER_EVT_FEEDBACK_SUPPORT:
        {
            app_usb_audio_set_feedback_state(USB_FEEDBACK_SUPPORT);

            app_usb_feedback_detect_stop();
        }
        break;
#endif

    case USB_OTHER_EVT_AUDIO_USB_STREAM_READY_TO_TX:
        {
            app_usb_set_stream_state_to_tx(data);
            app_ipc_publish(USB_IPC_TOPIC, USB_IPC_EVT_AUDIO_DS_XMIT, NULL);
        }
        break;

#if F_APP_GAMING_DONGLE_DELAY_TX_WHEN_BT_PLAYING
    case USB_OTHER_EVT_DELAY_USB_TX_START:
        {
            app_usb_delay_usb_tx_start();
        }
        break;
#endif

    case USB_OTHER_EVT_SILENCE_DETECT:
        {
            app_usb_set_stream_silence_for_a_while((bool)data);
        }
        break;

    default:
        break;
    }
}

void app_usb_msg_handle(T_IO_MSG *msg)
{
    uint16_t subtype = msg->subtype;
    uint8_t group = USB_MSG_GROUP(subtype);
    uint8_t submsg = USB_MSG_SUBTYPE(subtype);
    uint32_t param = msg->u.param;

    switch (group)
    {
    case USB_MSG_GROUP_DEV:
        {
            app_usb_dm_evt_handle(submsg, param);
        }
        break;

#if F_APP_USB_AUDIO_SUPPORT
    case USB_MSG_GROUP_IF_AUDIO:
        {
            usb_audio_stream_evt_handle(submsg, param);
        }
        break;
#endif

    case USB_MSG_GROUP_IF_OTHER:
        {
            app_usb_other_evt_handle(submsg, param);
        }
        break;

#if F_APP_USB_HID_SUPPORT
    case USB_MSG_GROUP_HOST_DETECT:
        {
            usb_host_detect_evt_handle(submsg, param);
        }
        break;
#endif
    default:
        break;
    }
}

#endif

bool app_usb_dvfs_check(void)
{
    return (app_usb_db.usb_pwr_state == USB_PDN || app_usb_db.usb_pwr_state == USB_SUSPENDED);
}

void app_usb_init(void)
{
    memset(&app_usb_db, 0, sizeof(T_APP_USB_DB));

    app_timer_reg_cb(app_usb_timer_cback, &app_usb_timer_queue_id);

    app_usb_cfg_device_desc();
    app_usb_cfg_string();
    pm_dvfs_register_check_func(app_usb_dvfs_check);
    adp_register_state_change_cb(ADP_DETECT_5V, (P_ADP_PLUG_CBACK)app_usb_adp_state_change_cb, NULL);
    T_USB_DM_EVT_MSK evt_msk = {0};
    evt_msk.b.status_ind = 1;
    evt_msk.b.bc12_det = 1;
    usb_dm_cb_register(evt_msk, app_usb_dm_cb);
#if F_APP_USB_GIP_SUPPORT
    usb_gip_driver_evt_cb_register(app_usb_gip_cb);
#endif
    T_USB_CORE_CONFIG config = {.speed = app_cfg_const.dongle_usb_speed, .class_set = {.hid_enable = 0, .uac_enable = 0}};
#if F_APP_USB_GIP_SUPPORT
    if (usb_gip_flag)
    {
        config.class_set.gip_enable = 1;
    }
    else
#endif
    {
        config.class_set.uac_enable = app_cfg_const.dongle_usb_audio_support;
        config.class_set.hid_enable = app_cfg_const.dongle_usb_hid_support;
    }
    usb_dm_core_init(config);
    usb_dev_init();

#if F_APP_USB_HID_SUPPORT && F_APP_USB_AUDIO_SUPPORT && (USB_AUDIO_VERSION == USB_AUDIO_VERSION_1)
    usb_host_detect_init();

    usb_host_detect_cback_register(app_usb_host_detect_cb);
#endif
    app_usb_vol_control_init();

    if (app_cfg_const.dongle_usb_audio_support)
    {
        app_usb_uac_init();
    }
#if F_APP_USB_GIP_SUPPORT
    if (usb_gip_flag)
    {
    }
    else
#endif
    {
        if (app_cfg_const.dongle_usb_hid_support)
        {
            app_usb_hid_init();
        }
    }

#if F_APP_GAMING_CONTROLLER_SUPPORT
    app_usb_controller_init();
#endif

#if (UAC_SILENCE_DETECT_SUPPORT == 1)
    app_silence_detect_init();
#endif
}


#if F_APP_USB_GIP_SUPPORT
void app_usb_set_gip_flag(uint8_t value)
{
    usb_gip_flag = value;
}
#endif
#endif
