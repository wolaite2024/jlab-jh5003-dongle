/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_IO_MSG_H_
#define _APP_IO_MSG_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_flags.h"
#include "app_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define IO_MSG_TYPE_DONGLE_APP       0xF6

typedef struct
{
    uint8_t   bd_addr[6];
    uint8_t   bd_type;
} T_RMV_BOND_DEV;


typedef enum
{
    REMOVE_BOND_EVENT               = 1,
    APP_LE_AUDIO_MSG_SEND_EVENT     = 2,
#if (UAC_SILENCE_DETECT_SUPPORT == 1)
    SILENCE_DETECT_EVENT             = 3,
#if (ENABLE_UAC2 == 1)
    UAC2_SILENCE_DETECT_EVENT        = 4,
#endif
#endif
    GIP_READ_I2C_STATE              = 5,
    GIP_READ_DATA                   = 6,
} T_APP_EVENT;

/**
 * @brief send io msg to app task
 * \xrefitem Added_API_2_13_0_0 "Added Since 2.13.0.0" "Added API"
 * @param io_msg io msg
 * @return true send ok
 * @return false send fail
 */
bool app_io_msg_send(T_IO_MSG *io_msg);

/** @defgroup APP_IO_MSG App IO Msg
  * @brief App IO Msg
  * @{
  */
extern void (*app_rtk_charger_box_dat_msg_hook)(T_IO_MSG *);

/**
    * @brief  All the application events are pre-handled in this function.
    *         All the io messages are sent to this function.
    *         Then the event handling function shall be called according to the message type.
    * @param  io_driver_msg_recv The T_IO_MSG from peripherals or BT stack state machine.
    * @return void
    */
void app_io_handle_msg(T_IO_MSG io_driver_msg_recv);


/** End of APP_IO_MSG
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_IO_MSG_H_ */
