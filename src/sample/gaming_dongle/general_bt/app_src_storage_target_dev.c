#include "app_src_storage_target_dev.h"

#include "ftl.h"
#include <string.h>
#include <stdint.h>
#include <trace.h>

#include "app_ctrl_cfg.h"

#define APP_SRC_FLASH_TARFET_DEVICES_INFO_OFFSET    4500

#define APP_SRC_FLASH_TARGET_DEVICE_SIZE            (sizeof(T_APP_SRC_TARGET_DEV))

static uint32_t app_src_flash_save_target_device(T_APP_SRC_TARGET_DEV *p_data, uint8_t idx)
{
    uint16_t offset;
    uint8_t size;

    offset = APP_SRC_FLASH_TARFET_DEVICES_INFO_OFFSET +
             idx * APP_SRC_FLASH_TARGET_DEVICE_SIZE;

    size = sizeof(T_APP_SRC_TARGET_DEV);

    GAP_PRINT_TRACE3("app_src_flash_save_target_device: idx %d, addr %s,addr_type %d",
                     idx, TRACE_BDADDR(p_data->remote_bd), p_data->addr_type);

    return ftl_save_to_storage(p_data, offset, size);
}

static uint32_t app_src_flash_load_target_device(T_APP_SRC_TARGET_DEV *p_data, uint8_t idx)
{
    uint16_t offset;
    uint32_t has_error;
    uint8_t size;

    GAP_PRINT_TRACE1("app_src_flash_load_target_device: idx %d", idx);

    offset = APP_SRC_FLASH_TARFET_DEVICES_INFO_OFFSET +
             idx * APP_SRC_FLASH_TARGET_DEVICE_SIZE;

    size = sizeof(T_APP_SRC_TARGET_DEV);

    has_error =  ftl_load_from_storage(p_data, offset, size);

    if (has_error)
    {
        memset(p_data, 0, size);
    }

    return has_error;
}

bool app_src_storage_set_target_dev(T_APP_SRC_TARGET_DEV target_dev)
{
    uint8_t index = 0;

    if ((target_dev.dev_type > DEV_SLOT_HOGP_2) ||
        (target_dev.bt_mode > DEV_MODE_LE) || (target_dev.addr_type > 1))
    {
        return false;
    }

    index = target_dev.dev_type - 1;

    target_dev.is_valid = 1;

    if (app_src_flash_save_target_device(&target_dev, index) != 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool app_src_storage_get_target_dev(uint8_t dev_type, T_APP_SRC_TARGET_DEV *target_dev)
{
    uint8_t index = 0;
    T_APP_SRC_TARGET_DEV temp_dev;

    if ((dev_type > 3) || (target_dev == NULL))
    {
        return false;
    }

    APP_PRINT_INFO1("app_src_storage_get_target_dev: %d", dev_type);

    index = dev_type - 1;

    if ((app_src_flash_load_target_device(&temp_dev, index) == 0) &&
        (temp_dev.is_valid == 1))
    {
        memcpy(target_dev, &temp_dev, sizeof(temp_dev));
        return true;
    }
    else
    {
        return false;
    }
}

bool app_src_storage_clear_target_dev(uint8_t dev_type)
{
    if (dev_type > 3)
    {
        return false;
    }

    T_APP_SRC_TARGET_DEV temp_dev;
    memset(&temp_dev, 0xff, sizeof(T_APP_SRC_TARGET_DEV));
    if (dev_type == 0)
    {
        for (uint8_t idx = 0; idx < 3; idx++)
        {
            if (app_src_flash_save_target_device(&temp_dev, idx) != 0)
            {
                return false;
            }
        }
    }
    else
    {
        uint8_t idx = dev_type - 1;
        if (app_src_flash_save_target_device(&temp_dev, idx) != 0)
        {
            return false;
        }
    }
    return true;
}
