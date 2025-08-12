/*
 * Copyright (c) 2022, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CONSOLE_H_
#define _APP_CONSOLE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "app_msg.h"

/**
 * @brief receive data, uart rx wake up
 *
 * @param io_driver_msg_recv refs T_IO_MSG
 */
void app_console_uart_handle_msg(T_IO_MSG *io_driver_msg_recv);

/** @defgroup APP_CONSOLE_UART APP CONSOLE UART.
  * @brief app console uart event handle and implementation
  * @{
  */

/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup APP_CONSOLE_UART_Exported_Macros App Console UART Macros
    * @{
    */

/** End of APP_CONSOLE_UART_Exported_Macros
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup APP_CONSOLE_UART_Exported_Types App Console UART Types
    * @{
    */

/**  @brief  App define global app data structure */

/** End of APP_CONSOLE_UART_Exported_Types
    * @}
    */

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_CONSOLE_UART_Exported_Variables App Console UART Variables
    * @{
    */


/** End of APP_CONSOLE_UART_Exported_Variables
    * @}
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup APP_CONSOLE_UART_Exported_Variables App Console UART Functions
    * @{
    */

void app_console_init(void);

/** End of APP_CONSOLE_UART_Exported_Variables
    * @}
    */

/** End of APP_CONSOLE_UART
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CONSOLE_H_ */
