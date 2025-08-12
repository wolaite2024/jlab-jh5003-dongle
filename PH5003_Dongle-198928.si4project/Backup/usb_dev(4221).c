#if F_APP_USB_AUDIO_SUPPORT|F_APP_USB_HID_SUPPORT
#include <stddef.h>
#include "usb_dev_driver.h"
#include "usb_dev.h"
#include "trace.h"
#include "usb_audio_config.h"

#if (USB_AUDIO_VERSION == USB_AUDIO_VERSION_2)
#define IAD_SUPPORT     1
#else
#define IAD_SUPPORT     0
#endif

#define STRING_ID_MICROSOFT_OS  (0xEE)

static T_USB_DEVICE_DESC usb_dev_desc =
{
    .bLength                = sizeof(T_USB_DEVICE_DESC),
    .bDescriptorType        = USB_DESC_TYPE_DEVICE,

    .bcdUSB                 = 0x0200,
#if (IAD_SUPPORT == 1)
    .bDeviceClass           = 0xEF,
    .bDeviceSubClass        = 0x02,
    .bDeviceProtocol        = 0x01,
#else
    .bDeviceClass           = 0x00,
    .bDeviceSubClass        = 0x00,
    .bDeviceProtocol        = 0x00,
#endif
    .bMaxPacketSize0        = 0x40,
    .idVendor               = 0x0bda,
    .idProduct              = 0x8773,
    .bcdDevice              = 0x0200,
    .iManufacturer          = STRING_ID_MANUFACTURER,
    .iProduct               = STRING_ID_PRODUCT,
    .iSerialNumber          = STRING_ID_SERIALNUM,
    .bNumConfigurations     = 1,
};

void usb_dev_desc_rcfg(T_USB_DEV_DESC_CFG dev_desc_cfg)
{
    if (dev_desc_cfg.idProduct)
    {
        usb_dev_desc.idProduct = dev_desc_cfg.idProduct;
    }

    if (dev_desc_cfg.idVendor)
    {
        usb_dev_desc.idVendor = dev_desc_cfg.idVendor;
    }

    if (dev_desc_cfg.bcdDevice)
    {
        usb_dev_desc.bcdDevice = dev_desc_cfg.bcdDevice;
    }
#if F_APP_USB_GIP_SUPPORT
    if (dev_desc_cfg.bDeviceClass)
    {
        usb_dev_desc.bDeviceClass = dev_desc_cfg.bDeviceClass;
    }

    if (dev_desc_cfg.bDeviceSubClass)
    {
        usb_dev_desc.bDeviceSubClass = dev_desc_cfg.bDeviceSubClass;
    }

    if (dev_desc_cfg.bDeviceProtocol)
    {
        usb_dev_desc.bDeviceProtocol = dev_desc_cfg.bDeviceProtocol;
    }
#endif
}

static T_STRING dev_strings[] =
{
    [STRING_IDX_MANUFACTURER] =
    {
        .id     = STRING_ID_MANUFACTURER,
        .s = "RealTek",
    },

    [STRING_IDX_PRODUCT] =
    {
        .id     = STRING_ID_PRODUCT,
        .s = "USB Audio Device",
    },

    [STRING_IDX_SERIALNUM] =
    {
        .id     = STRING_ID_SERIALNUM,
        .s = "0123456789A",
    },

    [STRING_IDX_TEAMS_UCQ] =
    {
        .id     = STRING_ID_TEAMS_UCQ,
        .s = "UCQ00011000101000",
    },

    [STRING_IDX_UAC_1st] =
    {
        .id     = STRING_ID_UAC_1st,
        .s = "RTL8763EAU - Chat",
    },

    [STRING_IDX_UAC_2st] =
    {
        .id     = STRING_ID_UAC_2st,
        .s = "RTL8763EAU - Game",
    },

    [STRING_IDX_MAX] =
    {
        .id     = STRING_ID_UNDEFINED,
        .s = NULL,
    },
};

T_STRING_TAB dev_stringtab =
{
    .language = 0x0409,
    .strings = dev_strings,
};

char microsoft_os_desc[] = {'M', 'S', 'F', 'T', '1', '0', '0', 0x90, '\0'};

static T_STRING dev_strings_ms[] =
{
    [0] =
    {
        .id     = STRING_ID_MICROSOFT_OS,
        .s = microsoft_os_desc,
    },

    [1] =
    {
        .id     = STRING_ID_UNDEFINED,
        .s = NULL,
    },
};

T_STRING_TAB dev_stringtab_ms =
{
    .language = 0x0000,
    .strings = dev_strings_ms,
};

const T_STRING_TAB *const dev_stringtabs[] =
{
    [0] = &dev_stringtab,
    [1] = &dev_stringtab_ms,
    [2] = NULL,
};

bool usb_dev_desc_string_rcfg(T_STRING_IDX index, const char *s)
{
    USB_PRINT_TRACE2("usb_device_desc_string_rcfg, index %d,strlen %s", index, TRACE_STRING(s));

    if (index >= STRING_IDX_MAX)
    {
        return false;
    }

    dev_strings[index].s = s;

    return true;
}

static T_USB_CONFIG_DESC usb_cfg_desc =
{
    .bLength = sizeof(T_USB_CONFIG_DESC),
    .bDescriptorType = USB_DESC_TYPE_CONFIG,
    .wTotalLength = 0xFFFF, //wTotalLength will be recomputed in usb lib according total interface descriptors
    .bNumInterfaces = 3, //bNumInterfaces will be recomputed in usb lib according total interface num
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 0x32
};

void usb_dev_init(void)
{
    usb_dev_driver_dev_desc_register((void *)&usb_dev_desc);
    usb_dev_driver_cfg_desc_register((void *)&usb_cfg_desc);
    usb_dev_driver_string_desc_register((void *)dev_stringtabs);
}
#endif
