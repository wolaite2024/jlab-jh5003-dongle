#if F_APP_CUSTOMIZED_SBC_VP_FLOW_SUPPORT
#include <string.h>
#include <stdlib.h>
#include "trace.h"
#include "fmc_api.h"
#include "app_main.h"
#include "app_timer.h"
#include "app_dongle_data_ctrl.h"
#include "app_dongle_customized_vp.h"

typedef enum
{
    ERASE_RSP_SUCCESS,
    ERASE_RSP_FAIL,
    ERASE_RSP_TIMEOUT,
    ERASE_RSP_DISALLOW_ILLEGAL_ADDR,
} T_ERASE_RSP;

typedef enum
{
    WRITE_RSP_SUCCESS,
    WRITE_RSP_FAIL,
    WRITE_RSP_TIMEOUT,
    WRITE_RSP_DISALLOW_ILLEGAL_ADDR,
} T_WRITE_RSP;

typedef enum
{
    APP_TIMER_ERASE_CUSTOMIZED_VP,
    APP_TIMER_WRITE_CUSTOMIZED_VP,
} T_CUSTOMIZED_VP_TIMER;

static uint8_t customized_vp_timer_id = 0;
static uint8_t timer_idx_erase_customized_vp = 0;
static uint8_t timer_idx_write_customized_vp = 0;

static uint32_t cur_write_offset = 0;
static uint32_t base_addr = 0;

static bool app_write_customized_vp(uint8_t *data, uint16_t length)
{
    bool ret = false;
    uint32_t write_addr = base_addr + cur_write_offset;

    ret = fmc_flash_nor_write(write_addr, data, length);

    if (ret)
    {
        cur_write_offset += length;
    }

    APP_PRINT_TRACE5("app_write_customized_vp: %d, %x, %x, %x, %x", ret, base_addr, write_addr,
                     length, (base_addr + cur_write_offset));

    return ret;
}

static void app_clear_info(void)
{
    base_addr = 0;
    cur_write_offset = 0;
}

static void app_customized_vp_timeout_cb(uint8_t timer_evt, uint16_t param)
{
    APP_PRINT_TRACE2("app_customized_vp_timeout_cb: timer id 0x%02X, ch 0x%x", timer_evt, param);

    switch (timer_evt)
    {
    case APP_TIMER_ERASE_CUSTOMIZED_VP:
        {
            app_stop_timer(&timer_idx_erase_customized_vp);

            //Send erase timeout rsp to dongle.
            T_ERASE_RSP rsp = ERASE_RSP_TIMEOUT;

            app_dongle_send_cmd(DONGLE_CMD_ERASE_FLASH, &rsp, 1);
        }
        break;

    case APP_TIMER_WRITE_CUSTOMIZED_VP:
        {
            app_stop_timer(&timer_idx_write_customized_vp);

            //Send write timeout rsp to dongle.
            T_WRITE_RSP rsp = WRITE_RSP_TIMEOUT;

            app_dongle_send_cmd(DONGLE_CMD_WRITE_CUSTOMIZED_VP, &rsp, 1);
        }
        break;

    default:
        break;
    }
}

void app_dongle_receive_customized_vp_data(uint16_t data_len, uint8_t *p_data)
{
    T_WRITE_RSP rsp = WRITE_RSP_SUCCESS;

    app_start_timer(&timer_idx_write_customized_vp, "write_customized_vp",
                    customized_vp_timer_id, APP_TIMER_WRITE_CUSTOMIZED_VP, 0, false,
                    3000);

    //firt 4 bytes are base address in flash.
    LE_ARRAY_TO_UINT32(base_addr, p_data);

    if (base_addr < BKP2_FLASH_ADDR)
    {
        rsp = WRITE_RSP_DISALLOW_ILLEGAL_ADDR;
    }
    else
    {
        if (app_write_customized_vp(p_data + 4, data_len - 4) == false)
        {
            rsp = WRITE_RSP_FAIL;
            APP_PRINT_ERROR0("Write flash fail!");
        }
    }

    app_stop_timer(&timer_idx_write_customized_vp);

    //Send write rsp to dongle.
    app_dongle_send_cmd(DONGLE_CMD_WRITE_CUSTOMIZED_VP, &rsp, 1);
}

void app_dongle_write_vp_finish(void)
{
    APP_PRINT_TRACE0("app_dongle_write_vp_finish");

    app_clear_info();

    //Do other operations when write finish if needed
}

void app_dongle_receive_erase_cmd(uint8_t *p_data)
{
    uint32_t addr;
    T_ERASE_RSP rsp = ERASE_RSP_SUCCESS;

    app_start_timer(&timer_idx_erase_customized_vp, "erase_customized_vp",
                    customized_vp_timer_id, APP_TIMER_ERASE_CUSTOMIZED_VP, 0, false,
                    1000);

    LE_ARRAY_TO_UINT32(addr, p_data);

    APP_PRINT_TRACE1("app_dongle_receive_erase_cmd: addr 0x%x", addr);

    if (addr < BKP2_FLASH_ADDR)
    {
        rsp = ERASE_RSP_DISALLOW_ILLEGAL_ADDR;
    }
    else
    {
        // One sector size is equal to 4K.
        if (fmc_flash_nor_erase(addr, FMC_FLASH_NOR_ERASE_SECTOR) == false)
        {
            rsp = ERASE_RSP_FAIL;
            APP_PRINT_ERROR0("Erase flash fail!");
        }
    }

    app_stop_timer(&timer_idx_erase_customized_vp);

    //Send erase rsp to dongle.
    app_dongle_send_cmd(DONGLE_CMD_ERASE_FLASH, &rsp, 1);
}

void app_dongle_customized_vp_init(void)
{
    app_timer_reg_cb(app_customized_vp_timeout_cb, &customized_vp_timer_id);
}
#endif
