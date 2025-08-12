/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdbool.h>
#include <string.h>
#include "sys_ver.h"

#define SYSM_VERSION_INFO v1_6_1_2
GENERATE_BIN_VERSION(framework_lib, SYSM_VERSION_INFO)

/* Framework release version */
const char sysm_version[] = TO_STR(SYSM_VERSION_INFO);

/* Framework commit version created by the script */
const char sysm_commit[] = TO_STR(COMMIT);

/* Framework build date created by the script */
const char sysm_build_date[] = TO_STR(BUILDING_TIME);

bool sys_mgr_version_get(char ver[],
                         size_t ver_len,
                         char commit[],
                         size_t commit_len,
                         char build_date[],
                         size_t date_len)
{
    if (ver_len == 0 || commit_len == 0 || date_len == 0)
    {
        return false;
    }

    strncpy(ver, sysm_version, ver_len);
    ver[ver_len - 1] = '\0';

    strncpy(commit, sysm_commit, commit_len);
    commit[commit_len - 1] = '\0';

    strncpy(build_date, sysm_build_date, date_len);
    build_date[date_len - 1] = '\0';

    return true;
}
