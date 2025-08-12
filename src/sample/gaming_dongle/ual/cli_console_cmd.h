/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _CLI_CONSOLE_CMD_H_
#define _CLI_CONSOLE_CMD_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BS_SET_CONFIGURE    0x01
#define UC_SET_CONFIGURE    0X02
#define UC_SET_CONFIGURE2   0x03
#define UC_SET_CONFIGURE3   0x04
#define UC_SET_CONFIGURE4   0x05
#define UC_SET_CONFIGURE5   0x06
#define UC_SET_CONFIGURE6   0x07

bool console_cmd_register(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CLI_ISOC_H_ */
