/*
 * Copyright (c) 2021, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "app_status_report.h"
#include "trace.h"

#if APP_DEBUG_REPORT

T_APP_STATUS_INFO app_status_report;

void app_status_report_print()
{

}


void app_status_report_init()
{
    memset(&app_status_report, 0, sizeof(T_APP_STATUS_INFO));
}
#endif
