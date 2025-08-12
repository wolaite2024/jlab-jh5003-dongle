#include "app_bqb_mode_switch.h"
//#include "string.h"
#include "ftl.h"
#include "trace.h"

#ifdef      BTDONGLE_BQB_MODE_ENABLE

#define     APP_BQB_MODE_FLAG_FTL_OFFSET           (4796)
#define     APP_BQB_MODE_FLAG_SIZE                  (4)


static bool bqb_mode_enable = false;


static uint8_t bqb_mode_mask[APP_BQB_MODE_FLAG_SIZE] = {0x55, 0xAA, 0x55, 0xAA};

bool app_is_bqb_mode(void)
{
    return bqb_mode_enable;
}

static bool bqb_mode_ftl_data_eq_mask(uint8_t *ftl_data)
{
    for (uint8_t i = 0; i < APP_BQB_MODE_FLAG_SIZE; i++)
    {
        if (ftl_data[i] != bqb_mode_mask[i])
        {
            return false;
        }
    }
    return true;
}

void app_bqb_mode_load(void)
{
    uint8_t ftl_data[APP_BQB_MODE_FLAG_SIZE] = {0};
    uint32_t ret = ftl_load_from_storage(ftl_data, APP_BQB_MODE_FLAG_FTL_OFFSET,
                                         APP_BQB_MODE_FLAG_SIZE);
    if (ret)
    {
        bqb_mode_enable = false;
    }
    else
    {
        if (bqb_mode_ftl_data_eq_mask(ftl_data))
        {
            bqb_mode_enable = true;
        }
        else
        {
            bqb_mode_enable = false;
        }
    }
}

void app_enable_bqb_mode(void)
{
    int ret = ftl_save_to_storage(bqb_mode_mask, APP_BQB_MODE_FLAG_FTL_OFFSET, APP_BQB_MODE_FLAG_SIZE);
    if (ret)
    {
        APP_PRINT_INFO0("app_enable_bqb_mode fail");
    }
    else
    {
        APP_PRINT_INFO0("app_enable_bqb_mode success");
    }
}

void app_disable_bqb_mode(void)
{
    uint8_t bqb_mode_disable[APP_BQB_MODE_FLAG_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t ret = ftl_save_to_storage(bqb_mode_disable, APP_BQB_MODE_FLAG_FTL_OFFSET,
                                       APP_BQB_MODE_FLAG_SIZE);
    if (ret)
    {
        APP_PRINT_INFO0("app_disable_bqb_mode fail");
    }
    else
    {
        APP_PRINT_INFO0("app_disable_bqb_mode success");
    }
}
#endif
