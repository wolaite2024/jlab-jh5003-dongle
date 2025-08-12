#include "app_vendor_cfg.h"
#include "string.h"
#include "app_usb_dongle_test.h"


#define APP_MS_CONFIG_ADDR          (MSFT_CONFIG_ADDR)

#define APP_MS_CFG_LEN              (32)

#define APP_MS_CFG_MASK_OFFSET      (24)

#define APP_MS_CFG_MASK_LEN         (8)

static bool cfg_is_audio_hid = false;

//static uint8_t cfg_local[APP_MS_CFG_LEN] = {0};

static uint8_t MS_CFG_AUDIO_HID_MASK[APP_MS_CFG_MASK_LEN] =
{
    0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x01, 0xff
};

/*
static uint8_t MS_CFG_AUDIO_ONLY_MASK[APP_MS_CFG_MASK_LEN] =
{
    0x00, 0x00, 0x00, 0x79, 0x00, 0x00, 0x00, 0x79
};

static uint8_t MS_CFG_DEFAULT_VAL[APP_MS_CFG_LEN] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x01,

    0x01,

    0x00, 0x00, 0x00, 0x79,

    0x00, 0x00, 0x00, 0x79
};
*/
static bool app_vendor_cfg_is_invalid(void)
{
    uint8_t i;
    uint8_t *pdata = (uint8_t *)APP_MS_CONFIG_ADDR;

    for (i = 0; i < APP_MS_CFG_LEN; i++)
    {
        if (pdata[i] != 0xff)
        {
            return false;
        }
    }

    return true;
}

static bool app_vendor_cfg_is_mask_type(uint8_t mask[8])
{
    uint8_t i;
    uint8_t *pdata = (uint8_t *)(APP_MS_CONFIG_ADDR + APP_MS_CFG_MASK_OFFSET);

    for (i = 0; i < APP_MS_CFG_MASK_LEN; i++)
    {
        if ((pdata[i] & mask[i]) != mask[i])
        {
            return false;
        }
    }

    return true;
}

void app_vendor_cfg_get_capability(uint8_t cap[32])
{
    //memcpy(cap, cfg_local, APP_MS_CFG_LEN);
    memcpy(cap, (uint8_t *)APP_MS_CONFIG_ADDR, APP_MS_CFG_LEN);
}

bool app_vendor_cfg_is_audio_hid(void)
{
    return cfg_is_audio_hid;
}

void app_vendor_cfg_load(void)
{
    //memcpy(cfg_local, (uint8_t *)APP_MS_CONFIG_ADDR, APP_MS_CFG_LEN);

    //cfg_is_audio_hid = app_vendor_cfg_is_mask_type(cfg_local, MS_CFG_AUDIO_HID_MASK);
    if (app_vendor_cfg_is_invalid())
    {
        cfg_is_audio_hid = false;
    }
    else
    {
        cfg_is_audio_hid = app_vendor_cfg_is_mask_type(MS_CFG_AUDIO_HID_MASK);
    }
}
