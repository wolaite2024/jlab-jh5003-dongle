
#include "app_timer.h"
#include "trace.h"
#include "app_cfg.h"
#include "app_main.h"
#include "app_link_util.h"
#include "app_ble_timer.h"
#include "app_ble_common_adv.h"

static uint8_t  app_ble_timer_id;
static uint8_t  timer_idx_check_common_link;
static uint32_t check_common_link_timer_timeout_ms = 15000;//15s

static uint8_t  current_ble_link_conn_id = 0xFF;

typedef enum
{
    APP_BLE_TIMER_CHECK_COMMON_LINK = 0x00,
    APP_BLE_TIMER_TOTAL,
} T_APP_BLE_TIMER;

/**
 * @brief start timer, when timeout check current ble link is common link or not
 * common link: this ble link is connected through common adv
 */
void app_ble_timer_start_check_common_link(void)
{
    app_start_timer(&timer_idx_check_common_link, "app_ble_timer_check_common_link",
                    app_ble_timer_id, APP_BLE_TIMER_CHECK_COMMON_LINK, 0,
                    false, check_common_link_timer_timeout_ms);
}

/**
 * @brief set current link connection id for check
 *
 * @param conn_id
 */
void app_ble_timer_set_current_link_conn_id(uint8_t conn_id)
{
    APP_PRINT_INFO1("app_ble_timer_set_current_link_conn_id: %d", conn_id);
    current_ble_link_conn_id = conn_id;
}

void app_ble_timer_stop_check_common_link(void)
{
    app_stop_timer(&timer_idx_check_common_link);
}

/**
 * @brief check current link is common link or not
 * "Common link" means that this BLE link is established through common adv
 */
bool app_ble_timer_check_common_link(uint8_t conn_id)
{
    bool ret = false;

    for (uint8_t i = 0; i < MAX_BLE_LINK_NUM; i++)
    {
        if (app_db.le_link[i].conn_id == conn_id)
        {
            if (app_db.le_link[i].is_common_link == true)
            {
                ret = true;
            }
        }
    }

    APP_PRINT_INFO1("app_ble_timer_check_common_link: %d", ret);

    return ret;
}

void app_ble_timer_timeout_cb(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_TRACE2("app_ble_timer_timeout_cb: timer_id %d, timer_chann %d", timer_id, timer_chann);

    switch (timer_id)
    {
    case APP_BLE_TIMER_CHECK_COMMON_LINK:
        {
            if (app_ble_timer_check_common_link(current_ble_link_conn_id) == false)
            {
                //reopen common adv
                if (app_cfg_const.timer_ota_adv_timeout == 0)
                {
                    /*always advertising*/
                    app_ble_common_adv_start_rws(0);
                }
                else if (app_cfg_const.enable_power_on_adv_with_timeout)
                {
                    /*advertisng with timeout*/
                    app_ble_common_adv_start_rws(app_cfg_const.timer_ota_adv_timeout * 100);
                }
            }
            else
            {

            }
        }
        break;

    default:
        break;
    }
}

void app_ble_timer_init(void)
{
    app_timer_reg_cb(app_ble_timer_timeout_cb, &app_ble_timer_id);
}
