/**
 * @copyright Copyright (C) 2022 Realtek Semiconductor Corporation.
 *
 * @file ipc.h
 * @version 1.0
 * @brief IPC-related definitions
 *
 * @note:
 */
#ifndef __IPC_H__
#define __IPC_H__
#include <stdint.h>

/**
 * @brief task to process ipc-related evt
 *
 * @param pvParameters
 */
void ipc_task(void *pvParameters);

#endif //__IPC_H__
