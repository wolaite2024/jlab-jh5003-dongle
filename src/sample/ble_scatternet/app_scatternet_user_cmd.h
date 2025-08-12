/**
*********************************************************************************************************
*               Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     app_scatternet_user_cmd.h
* @brief    Define user command.
* @details
* @author   jane
* @date     2016-02-18
* @version  v0.1
*********************************************************************************************************
*/
#ifndef _APP_SCATTERNET_USER_CMD_H_
#define _APP_SCATTERNET_USER_CMD_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "data_uart.h"
#include "user_cmd_parse.h"

/** @defgroup SCATTERNET_CMD Scatternet User Command
  * @brief Scatternet User Command
  * @{
  */
extern const T_USER_CMD_TABLE_ENTRY user_cmd_table[];
extern T_USER_CMD_IF    user_cmd_if;

/** End of SCATTERNET_CMD
* @}
*/

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
