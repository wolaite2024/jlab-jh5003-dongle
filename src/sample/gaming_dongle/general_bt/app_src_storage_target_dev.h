#ifndef _APP_SRC_STORAGE_TARGET_DEV_H_
#define _APP_SRC_STORAGE_TARGET_DEV_H_

#include <stdint.h>
#include <stdbool.h>
#include "gap.h"

#define TARGET_DEVICE_TYPE_HEADSET                    0x01
#define TARGET_DEVICE_TYPE_LE_SLOT1                   0x02
#define TARGET_DEVICE_TYPE_LE_SLOT2                   0x03

#define HARDWARE_ID_LEN_MAX         20

typedef struct
{
    uint8_t is_valid;
    uint8_t dev_type;
    uint8_t bt_mode;
    uint8_t addr_type;
    uint8_t remote_bd[6];
    uint8_t padding[2];
    uint32_t ms_dev_type;

    uint16_t vid;
    uint16_t pid;

    uint8_t hardware_id[HARDWARE_ID_LEN_MAX];
    uint8_t remote_name[GAP_DEVICE_NAME_LEN];
} T_APP_SRC_TARGET_DEV;

bool app_src_storage_get_target_dev(uint8_t dev_type, T_APP_SRC_TARGET_DEV *target_dev);

bool app_src_storage_set_target_dev(T_APP_SRC_TARGET_DEV target_dev);

bool app_src_storage_clear_target_dev(uint8_t dev_type);

#endif
