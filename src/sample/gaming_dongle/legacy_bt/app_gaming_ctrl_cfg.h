#ifndef _APP_GAMING_CTRL_CFG_H_
#define _APP_GAMING_CTRL_CFG_H_

#include "stdint.h"
#include "app_msg.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @defgroup APP_GAMING_CTRL_CFG App Gaming Ctl Cfg
  * @brief USB command interaction between PC and dongle.
  * @{
  */

/**
* app_gming_ctrl_cfg.h
*
* \brief App timer id.
*/
typedef enum
{
    CONNECTION_STATE_EVENT_TIMER_ID        = 0x01,
    CONNECTION_STATE_EVENT_TIMER_ID_2      = 0x02,
    HEADSET_LOCK_ACK_TIMER_ID              = 0x03,
} T_CTRL_TIMER_ID;

/**
* app_gming_ctrl_cfg.h
*
* \brief App gaming command status.
*/
#define GAMING_COMMAND_COMPLETE_SUCCESS             0x00
#define GAMING_UNKNOWN_COMMAND                      0x01
#define GAMING_INVALID_COMMAND_LENGTH               0x02
#define GAMING_INVALID_COMMAND_PARAM                0x03
#define GAMING_COMMAND_OPERATION_FAIL               0x04

/**
* app_gming_ctrl_cfg.h
*
* \brief App gaming command opcode.
*/
#define GAMING_RTK_START_OPCODE                     0x1000

//Legacy Gaming CMD
#define GAMING_BT_START_DISCOVERY_OPCODE            0x1001
#define GAMING_BT_STOP_DISCOVERY_OPCODE             0x1002
#define GAMING_BT_CONNECT_OPCODE                    0x1003
#define GAMING_BT_DISCONNECT_BY_ID_OPCODE           0x1004
#define GAMING_BT_DISCONNECT_BY_ADDRESS_OPCODE      0x1005
#define GAMING_BT_REMOVE_BOND_BY_ID_OPCODE          0x1006
#define GAMING_BT_REMOVE_BOND_BY_ADDRESS_OPCODE     0x1007
#define GAMING_BT_QUERY_CONNECTION_STATE_OPCODE     0x1008
#define GAMING_BT_GET_VERSION_OPCODE                0x1009
#define GAMING_BT_GET_DONGLE_NAME_OPCODE            0x100A
#define GAMING_BT_GET_PAIRED_DEVICE_INFO_OPCODE     0x100B
#define GAMING_BT_GET_PUBLIC_ADDRESS_OPCODE         0x100C
#define GAMING_BT_GET_LINK_KEY_OPCODE               0x100D
#define GAMING_BT_ENTER_DUT_MODE_OPCODE             0x100E
#define GAMING_BT_FACTORY_RESET_OPCODE              0x100F
#define GAMING_LOCK_DETECT_OPCODE                   0x1010
#define GAMING_LOCK_HEADSET_OPCODE                  0x1011
#define GAMING_DUAL_MODE_CONNECT_OPCODE             0x1013
#define GAMING_BT_MP_HCI_CMD_PASSTHROUGH_OPCODE     0x1014
#define GAMING_DEVICE_IDENTIFY_OPCODE               0x1015
#define GAMING_WRITE_ADDRESS_OPCODE                 0x1016
#define GAMING_READ_ADDRESS_OPCODE                  0x1017
#define GAMING_BT_PAIR_OPCODE                       0x1018
#define GAMING_BT_STOP_PAIR_OPCODE                  0x1019
#define GAMING_BT_GET_CONNECT_STATE                 0x101A
#define GAMING_BT_CANCEL_BOND	                    0x101B


/**
* app_gming_ctrl_cfg.h
*
* \brief App gaming event opcode.
*/
//Legacy Gaming Event Opcode
#define GAMING_BT_DISCOVERY_REPORT_EVENT            0x1E00
#define GAMING_BT_CONNECTION_STATE_REPORT_EVENT     0x1E01
#define GAMING_BT_DONGLE_VERSION_REPORT_EVENT       0x1E02
#define GAMING_BT_DONGLE_NAME_REPORT_EVENT          0x1E03
#define GAMING_BT_PAIRED_DEVICE_INFO_REPORT_EVENT   0x1E04
#define GAMING_BT_PUBLIC_ADDRESS_REPORT_EVENT       0x1E05
#define GAMING_BT_LINK_KEY_REPORT_EVENT             0x1E06
#define GAMING_LOCK_DETECT_REPORT_EVENT             0x1E07
#define GAMING_LOCK_HEADSET_REPORT_EVENT            0x1E08
#define GAMING_BT_DEVICE_TYPE_REPORT_EVENT          0x1E0A
#define GAMING_WRITE_ADDRESS_STATUS_EVENT           0x1E0B
#define GAMING_READ_ADDRESS_REPORT_EVENT            0x1E0C
#define GAMING_PAIR_STATUS_REPORT_EVENT             0x1E0D
#define GAMING_BT_CONNECT_STATE			            0x1E0E


//internal event
#define LEGACY_GAMING_COMMAND_COMPLETE_EVENT        0x1EFF


/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_Gaming_Ctrl_Cfg_Exported_Functions APP Gaming Ctrl Cfg Functions
  * @{
  */

/**
 * app_gaming_ctrl_cfg.h
 *
 * \brief   Process USB commands.
 *
 * \param[in] *p_data    USB command data.
 * \param[in] len        USB command data length.
 *
 * \return The status of process usb commands.
 * \retval 0x00     Complete command succeed.
 * \retval 0x01     ERROR - Unknow usb command.
 * \retval 0x02     ERROR - Invaild usb command length.
 * \retval 0x03     ERROR - Invaild usb command parameter.
 * \retval 0x03     ERROR - Operation usb command failed.
 */
uint8_t app_usb_hid_handle_gaming_cmd(uint8_t *p_data, uint16_t len);

/**
 * app_gaming_ctrl_cfg.h
 *
 * \brief   Gaming app control init.
 */
void gaming_app_ctrl_cfg_init(void);

/**
 * app_gaming_ctrl_cfg.h
 *
 * \brief   Gaming bt device status recording and processing.
 *
 * \param[in] event_type    State event type.
 * \param[in] *event_buf    Event buf data.
 * \param[in] buf_len       Event buf length.
 */
void gaming_usb_ctrl_cmd_msg(uint8_t event_type, uint8_t *event_buf, uint16_t buf_len);

/** @} */ /* End of group APP_Gaming_Ctrl_Cfg_Exported_Functions */

/** End of APP_GAMING_CTRL_CFG
* @}
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
