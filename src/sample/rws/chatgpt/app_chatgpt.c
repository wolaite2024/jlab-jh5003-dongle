/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_CHATGPT_SUPPORT
#include "app_chatgpt.h"

void app_chatgpt_init(void)
{
    app_chatgpt_ble_init();
    app_chatgpt_process_init();
}
#endif
