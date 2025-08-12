#include <stddef.h>
#include "usb_composite_dev.h"
#include "usb_dev_driver.h"

void usb_dev_driver_dev_desc_register(T_USB_DEVICE_DESC *desc)
{
    usb_composite_dev_init(desc);
}

void usb_dev_driver_string_desc_register(T_STRING_TAB *string_tbl[])
{
    uint16_t language = 0;
    uint8_t id = 0;
    const char *s = NULL;

    while (*string_tbl)
    {
        T_STRING *strings = (*string_tbl)->strings;
        language = (*string_tbl)->language;
        while (strings)
        {
            id = strings->id;
            s = strings->s;
            if (id != 0)
            {
                usb_composite_dev_string_add(language, id, s);
            }
            else
            {
                break;
            }
            strings++;
        }
        string_tbl++;
    }
}

void usb_dev_driver_dev_desc_unregister(void)
{
    usb_composite_dev_deinit();
}

void usb_dev_driver_string_desc_unregister(T_STRING_TAB *string_tbl[])
{
    uint16_t language = 0;
    uint8_t id = 0;
    const char *s = NULL;

    while (*string_tbl)
    {
        T_STRING *strings = (*string_tbl)->strings;
        language = (*string_tbl)->language;
        while (strings)
        {
            id = strings->id;
            s = strings->s;
            if (id != 0)
            {
                usb_composite_dev_string_remove(language, id, s);
            }
            else
            {
                break;
            }
            strings++;
        }
        string_tbl++;
    }
}

void usb_dev_driver_cfg_desc_register(T_USB_CONFIG_DESC *desc)
{
    usb_composite_dev_cfg_add(desc);
}

void usb_dev_driver_cfg_desc_unregister(T_USB_CONFIG_DESC *desc)
{
    usb_composite_dev_cfg_remove(desc);
}

void usb_dev_driver_vendor_cbs_register(T_USB_DEV_VENDOR_CBS *cbs)
{
    usb_composite_dev_vendor_cbs_register((T_USB_COMPOSITE_VENDOR_CBS *)cbs);
}

void usb_dev_driver_vendor_cbs_unregister(void)
{
    usb_composite_dev_vendor_cbs_unregister();
}

void usb_dev_driver_ctrl_xfer_buf_init(uint16_t buf_len)
{
    usb_composite_dev_ctrl_xfer_buf_init(buf_len);
    hal_usb_ep0_backup_rx_buf_init(buf_len);
}
