/**
*****************************************************************************************
*     Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_observer_gap.c
   * @brief     This file handles BLE observer application routines.
   * @author    danni
   * @date      2022-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2022 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include "trace.h"
#include "ble_mgr.h"
#include "string.h"
#include "app_observer_gap.h"
#include "app_observer_scan.h"

/** @defgroup  OB_APP Observer Application
    * @brief This file handles BLE observer application routines.
    * @{
    */
/** @defgroup  OB_GAP_MSG GAP Message Handler
    * @brief Handle GAP Message
    * @{
    */
/*============================================================================*
 *                              Variables
 *============================================================================*/
T_GAP_DEV_STATE gap_dev_state = {0, 0, 0, 0};/**< GAP device state */
void app_handle_gap_msg(T_IO_MSG  *p_gap_msg);

/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief app_gap_ble_mgr_init
 *        initialize ble manager lib which will enable ble extend scan module.
 */
void app_gap_ble_mgr_init(void)
{
    BLE_MGR_PARAMS param = {0};
    param.ble_scan.enable = true;
    ble_mgr_init(&param);
}

/**
 * @brief app_gap_init
 *        register gap message callback and initialize ble manager module.
 */
void app_gap_init(void)
{
    /* register gap message callback */
    le_register_app_cb(app_gap_callback);

    /* ble manager module initialize*/
    app_gap_ble_mgr_init();
}

/**
 * @brief app_handle_io_msg
 *        All the application messages are pre-handled in this function
 * @note  All the IO MSGs are sent to this function, then the event handling
 *        function shall be called according to the MSG type.
 * @param[in] io_msg  IO message data
 * @return    void
 */
void app_handle_io_msg(T_IO_MSG io_msg)
{
    uint16_t msg_type = io_msg.type;

    switch (msg_type)
    {
    case IO_MSG_TYPE_BT_STATUS:
        {
            app_handle_gap_msg(&io_msg);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief app_handle_dev_state_evt
 *        Handle msg GAP_MSG_LE_DEV_STATE_CHANGE
 * @note  All the gap device state events are pre-handled in this function.
 *        Then the event handling function shall be called according to the new_state.
 * @param[in] new_state  New gap device state
 * @param[in] cause GAP device state change cause
 * @return    void
 */
void app_handle_dev_state_evt(T_GAP_DEV_STATE new_state, uint16_t cause)
{
    APP_PRINT_INFO3("app_handle_dev_state_evt: init state %d, scan state %d, cause 0x%x",
                    new_state.gap_init_state,
                    new_state.gap_scan_state, cause);
    if (gap_dev_state.gap_init_state != new_state.gap_init_state)
    {
        if (new_state.gap_init_state == GAP_INIT_STATE_STACK_READY)
        {
            APP_PRINT_INFO0("app_handle_dev_state_evt: GAP stack ready");
            /*stack ready*/
            app_scan_start(APP_SCAN_INTERVAL, APP_SCAN_WINDOW);
        }
    }

    if (gap_dev_state.gap_scan_state != new_state.gap_scan_state)
    {
        if (new_state.gap_scan_state == GAP_SCAN_STATE_IDLE)
        {
            APP_PRINT_INFO0("GAP scan stop");
        }
        else if (new_state.gap_scan_state == GAP_SCAN_STATE_SCANNING)
        {
            APP_PRINT_INFO0("GAP scan start");
        }
    }

    gap_dev_state = new_state;
}

/**
 * @brief app_handle_gap_msg
 *        All the BT GAP MSG are pre-handled in this function.
 * @note  Then the event handling function shall be called according to the
 *        subtype of T_IO_MSG
 * @param[in] p_gap_msg Pointer to GAP msg
 * @return    void
 */
void app_handle_gap_msg(T_IO_MSG *p_gap_msg)
{
    T_LE_GAP_MSG gap_msg;
    memcpy(&gap_msg, &p_gap_msg->u.param, sizeof(p_gap_msg->u.param));
    ble_mgr_handle_gap_msg(p_gap_msg->subtype, &gap_msg);
    APP_PRINT_ERROR1("app_handle_gap_msg: subtype %d", p_gap_msg->subtype);

    switch (p_gap_msg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        {
            app_handle_dev_state_evt(gap_msg.msg_data.gap_dev_state_change.new_state,
                                     gap_msg.msg_data.gap_dev_state_change.cause);
        }
        break;

    default:
        //APP_PRINT_ERROR1("app_handle_gap_msg: unknown subtype %d", p_gap_msg->subtype);
        break;
    }
}
/** @} */ /* End of group OB_GAP_MSG */

/** @defgroup  OB_GAP_CALLBACK GAP Callback Event Handler
    * @brief Handle GAP callback event
    * @{
    */
/**
  * @brief app_gap_callback
  *        Callback for gap le to notify app
  * @param[in] cb_type callback msy type @ref GAP_LE_MSG_Types.
  * @param[in] p_cb_data point to callback data @ref T_LE_CB_DATA.
  * @retval    result @ref T_APP_RESULT
  */
T_APP_RESULT app_gap_callback(uint8_t cb_type, void *p_cb_data)
{
    T_APP_RESULT result = APP_RESULT_SUCCESS;
    T_LE_CB_DATA *p_data = (T_LE_CB_DATA *)p_cb_data;
    ble_mgr_handle_gap_cb(cb_type, p_cb_data);
    APP_PRINT_INFO1("app_gap_callback: cb_type 0x%x", cb_type);

    switch (cb_type)
    {
    case GAP_MSG_LE_MODIFY_WHITE_LIST:
        APP_PRINT_INFO2("app_gap_callback: GAP_MSG_LE_MODIFY_WHITE_LIST operation %d, cause 0x%x",
                        p_data->p_le_modify_white_list_rsp->operation,
                        p_data->p_le_modify_white_list_rsp->cause);
        break;

    default:
        //APP_PRINT_ERROR1("app_gap_callback: unhandled cb_type 0x%x", cb_type);
        break;
    }
    return result;
}
/** @} */ /* End of group OB_GAP_CALLBACK */

/** @} */ /* End of group OB_APP */
