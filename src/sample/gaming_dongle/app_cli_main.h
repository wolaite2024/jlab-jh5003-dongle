/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_CLI_H_
#define _APP_CLI_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define     APP_CMD  (0x2F)

#define GAMING_DISCOV_OPCODE        0x5dc1
#define GAMING_CONN_OPCODE          0x5dc2
#define GAMING_DISC_BY_ID_OPCODE    0x5dc3
#define GAMING_DISC_BY_ADDR_OPCODE  0x5dc4
#define GAMING_RM_BY_ID_OPCODE      0x5dc5
#define GAMING_RM_BY_ADDR_OPCODE    0x5dc6

typedef enum
{
    APP_ACTION_SCAN,
} app_action_t;


/** @defgroup APP_CLI
  * @brief App CLI
  * @{
  */

bool app_cmd_register(void);

/** End of APP_CLI
* @}
*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_CLI_H_ */
