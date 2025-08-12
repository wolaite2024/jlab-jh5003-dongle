/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include "trace.h"
#include "wdg.h"
#include "sys_cfg.h"
#include "fmc_api.h"

T_SYS_CFG_CONST sys_cfg_const;

void sys_cfg_load(void)
{
    uint32_t sync_word = 0;

    fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) + SYS_CONFIG_OFFSET,
                       &sync_word, DATA_SYNC_WORD_LEN);

    if (sync_word == DATA_SYNC_WORD)
    {
        fmc_flash_nor_read(flash_cur_bank_img_payload_addr_get(FLASH_IMG_MCUCONFIG) + SYS_CONFIG_OFFSET,
                           &sys_cfg_const, sizeof(T_SYS_CFG_CONST));
    }
    else
    {
        DBG_DIRECT("sys_cfg_load: failed");
        chip_reset(RESET_ALL);
    }
}
