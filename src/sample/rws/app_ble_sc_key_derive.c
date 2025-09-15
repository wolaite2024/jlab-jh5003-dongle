/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */
#if F_APP_SC_KEY_DERIVE_SUPPORT
#include "gap_bond_le.h"
#include "trace.h"
#include "app_ble_sc_key_derive.h"
#if F_APP_LEA_SUPPORT
#include "app_cfg.h"
#include "remote.h"
#endif

void app_ble_key_derive_init(void)
{
    APP_PRINT_INFO0("app_ble_key_derive_init");
    uint8_t init_key = SMP_DIST_ENC_KEY | SMP_DIST_ID_KEY | SMP_DIST_LINK_KEY;
    uint8_t response_key = SMP_DIST_ENC_KEY | SMP_DIST_LINK_KEY;

#if F_APP_RWS_BLE_USE_RPA_SUPPORT
    response_key = SMP_DIST_ENC_KEY | SMP_DIST_ID_KEY | SMP_DIST_LINK_KEY;
#endif

    le_bond_cfg_local_key_distribute(init_key, response_key);

    /*set convert LTK to linkkey*/
    uint8_t key_convert_flag = GAP_SC_KEY_CONVERT_LE_TO_BREDR_FLAG;

#if F_APP_LEA_SUPPORT
    if (app_cfg_nv.bud_role == REMOTE_SESSION_ROLE_SECONDARY)
    {
        key_convert_flag = GAP_SC_KEY_CONVERT_NONE;
    }
#if F_APP_LE_AUDIO_RWS_EN_SC
    key_convert_flag |= GAP_SC_KEY_CONVERT_BREDR_TO_LE_FLAG;
#endif
#endif
    gap_set_param(GAP_PARAM_BOND_LINK_KEY_CONVERT, sizeof(key_convert_flag), &key_convert_flag);
}
#endif
