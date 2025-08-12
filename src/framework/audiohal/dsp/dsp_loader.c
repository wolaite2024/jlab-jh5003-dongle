/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#include <stdbool.h>
#include <stdint.h>

#include "os_mem.h"
#include "bin_loader.h"
#include "dsp_mgr.h"
#include "dsp_ipc.h"
#include "dsp_loader.h"

/* TODO Remove Start */
#include "rtl876x_gdma.h"
#include "dsp_driver.h"

#if (TARGET_RTL8773D == 1)
#include "shm2_api.h"
#endif

typedef enum
{
    DSP_LOAD_STATE_INIT                 = 0x0,
    DSP_LOAD_STATE_IDLE                 = 0x1,
    DSP_LOAD_STATE_SYS                  = 0x2,
    DSP_LOAD_STATE_PATCH                = 0x3,
    DSP_LOAD_STATE_ALGO                 = 0x4,
    DSP2_LOAD_STATE_SYS                 = 0x5,

} T_DSP_LOAD_STATE;

extern uint8_t dsp_flash_dma_channel_num;
extern void dsp_mgr_load_finish(void);
extern bool dsp_anc_fw_load_code_driver(T_BIN_LOADER_SESSION_HANDLE  session,
                                        uint32_t                     id,
                                        void                        *context);

bool fix_scenario = false;

/* TODO Remove End */

#define DSP_FW_RELOAD_COUNT         5


typedef struct
{
    T_BIN_LOADER_SESSION_HANDLE *dsp_load_session;
    T_SHM_SCENARIO loader_scenario;
    T_DSP_LOAD_STATE state;
    uint8_t reload_count;
} T_DSP_LOAD_DB;

T_DSP_LOAD_DB *load_db = NULL;

void dsp_load_set_test_bin(bool enable)
{
    fix_scenario = enable;
}

bool dsp_load_init(void)
{
    int8_t ret = 0;

    if (load_db != NULL)
    {
        ret = 1;
        goto fail_check_db;
    }

    load_db = os_mem_zalloc2(sizeof(T_DSP_LOAD_DB));
    if (load_db == NULL)
    {
        ret = 2;
        goto fail_alloc_db;
    }

    load_db->state = DSP_LOAD_STATE_INIT;

    // remove dsp_drv_is_fpga_mode when bin loader policy is ready
    if (dsp_hal_is_fpga_mode())
    {
        load_db->loader_scenario = SHM_SCENARIO_SYS_ROM;
    }
    else
    {
        load_db->loader_scenario = SHM_SCENARIO_SYS_RAM;
    }

    return true;
fail_alloc_db:
fail_check_db:
    DIPC_PRINT_ERROR1("dsp_load_init: fail, ret %d", -ret);
    return false;
}

void dsp_load_deinit(void)
{
    if (load_db != NULL)
    {
        bin_loader_session_destory(load_db->dsp_load_session);
        os_mem_free(load_db);
        load_db = NULL;
    }
}

bool dsp_fw_cb_check_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                            uint32_t                     id,
                            void                        *context)
{
    dsp_send_msg(DSP_MSG_CHECK_LOAD_IMAGE_FINISH, 0, NULL, 0);
    return true;
}

bool dsp_fw_cb_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                      T_BIN_LOADER_EVENT           event,
                      void                        *context)
{
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
    if (dsp_load_get_scenario() == SHM_SCENARIO_DSP2_SYS_3)
    {
        if (event == BIN_LOADER_EVENT_SUCCESS)
        {
            dsp_hal_ready_set(READY_BIT_DSP2);
        }
    }
#endif
    dsp_send_msg(DSP_MSG_LOAD_IMAGE_FIHISH, 0, NULL, 0);
    return true;
}

T_SHM_SCENARIO dsp_loader_matched_scenario_get(T_DSP_SESSION_TYPE type)
{
    T_SHM_SCENARIO scenario = SHM_SCENARIO_A2DP;
    T_SHM_SCENARIO loader_scenario = dsp_load_get_scenario();

    if (type == DSP_SESSION_TYPE_VOICE)
    {
        scenario = SHM_SCENARIO_SCO;
    }
    else if ((type == DSP_SESSION_TYPE_TONE) || (type == DSP_SESSION_TYPE_VP))
    {
        if ((loader_scenario == SHM_SCENARIO_SCO) ||
            (loader_scenario == SHM_SCENARIO_A2DP))
        {
            scenario = loader_scenario;
        }
    }

    if (fix_scenario == true)
    {
        scenario = SHM_SCENARIO_PROPRIETARY_VOICE; /* for mp test */
    }

    DIPC_PRINT_TRACE3("dsp_loader_matched_scenario_get: scenario %d, loader_scenario %d, type %d",
                      scenario, loader_scenario, type);
    return scenario;
}

bool dsp_loader_bin_match(T_DSP_SESSION_TYPE type)
{
    T_SHM_SCENARIO loader_scenario = dsp_load_get_scenario();

    if (dsp_loader_matched_scenario_get(type) != loader_scenario)
    {
        return false;
    }

    return true;
}

bool dsp_load_algorithm_code(T_DSP_SESSION_TYPE type)
{
    T_SHM_SCENARIO scenario = dsp_loader_matched_scenario_get(type);

    DIPC_PRINT_TRACE4("dsp_load_algorithm_code: type %d, new scenario %d, old scenario %d, fix_scenario %d",
                      type, scenario, load_db->loader_scenario, fix_scenario);

    if (fix_scenario == true)
    {
        scenario = SHM_SCENARIO_PROPRIETARY_VOICE; /* for mp test */
    }

    if (scenario != load_db->loader_scenario)
    {
        load_db->loader_scenario = scenario;
        bin_loader_token_issue(load_db->dsp_load_session, scenario, NULL);
        return true;
    }
    else
    {
        dsp_fw_cb_finish(NULL, BIN_LOADER_EVENT_SUCCESS, NULL);
        return false;
    }
}

void dsp_load_set_scenario(T_SHM_SCENARIO scenario)
{
    load_db->loader_scenario = scenario;
}

void dsp_load_next_bin(void)
{
    T_SHM_SCENARIO scenario = dsp_load_get_scenario();

    if (bin_loader_driver_check_image_correct(scenario) == 0)
    {
        if ((scenario < SHM_SCENARIO_SYS_PATCH) &&
            (load_db->reload_count <= DSP_FW_RELOAD_COUNT))
        {
            load_db->reload_count++;
            DIPC_PRINT_TRACE0("bin_loader_driver_check_image_correct: CRC check fail");
//            dsp_hal_reset();
//            bin_loader_token_issue(load_db->dsp_load_session, scenario, NULL);
        }
    }
    else
    {
        load_db->reload_count = 0;
    }

    DIPC_PRINT_TRACE3("bin_loader_driver_check_image_correct: reload cnt %d, scenario %d, load_session%p",
                      load_db->reload_count, scenario, load_db->dsp_load_session);
}

T_SHM_SCENARIO dsp_load_get_scenario(void)
{
    return load_db->loader_scenario;
}

void dsp_load_finish(void)
{
    uint32_t algorithm;

    T_SHM_SCENARIO scenario = dsp_load_get_scenario();

    if (scenario == SHM_SCENARIO_SYS_ROM)
    {
        algorithm = SHM_SCENARIO_SYS_RAM;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_SYS_RAM);
        load_db->state = DSP_LOAD_STATE_SYS;
    }
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
    else if (scenario == SHM_SCENARIO_SYS_RAM)
    {
        DIPC_PRINT_TRACE0("dsp2_load_image_start");
        algorithm = SHM_SCENARIO_SYS_RAM_1;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_SYS_RAM_1);
        load_db->state = DSP_LOAD_STATE_SYS;
    }
    else if (scenario == SHM_SCENARIO_SYS_RAM_1)
    {
        algorithm = SHM_SCENARIO_SYS_RAM_2;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_SYS_RAM_2);
        load_db->state = DSP_LOAD_STATE_SYS;
    }
    else if (scenario == SHM_SCENARIO_SYS_RAM_2)
    {
        algorithm = SHM_SCENARIO_DSP2_SYS_1;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_DSP2_SYS_1);
        load_db->state = DSP2_LOAD_STATE_SYS;
    }
    //else if (scenario == SHM_SCENARIO_SYS_RAM)
    //{
    //    algorithm = SHM_SCENARIO_DSP2_SYS_1;
    //    bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
    //    dsp_load_set_scenario(SHM_SCENARIO_DSP2_SYS_1);
    //    load_db->state = DSP2_LOAD_STATE_SYS;
    //}
    else if (scenario == SHM_SCENARIO_DSP2_SYS_1)
    {
        algorithm = SHM_SCENARIO_DSP2_SYS_2;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_DSP2_SYS_2);
        load_db->state = DSP2_LOAD_STATE_SYS;
    }
    else if (scenario == SHM_SCENARIO_DSP2_SYS_2)
    {
        algorithm = SHM_SCENARIO_DSP2_SYS_3;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_DSP2_SYS_3);
        load_db->state = DSP2_LOAD_STATE_SYS;
    }
    else if (scenario == SHM_SCENARIO_DSP2_SYS_3)
    {
        DIPC_PRINT_TRACE0("dsp2_load_image_end");
        algorithm = SHM_SCENARIO_SYS_PATCH;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_SYS_PATCH);
        load_db->state = DSP_LOAD_STATE_PATCH;
    }
#else
    else if (scenario == SHM_SCENARIO_SYS_RAM)
    {
        algorithm = SHM_SCENARIO_SYS_PATCH;
        bin_loader_token_issue(load_db->dsp_load_session, algorithm, NULL);
        dsp_load_set_scenario(SHM_SCENARIO_SYS_PATCH);
        load_db->state = DSP_LOAD_STATE_PATCH;
    }

#endif
    else
    {
        if (scenario == SHM_SCENARIO_SYS_PATCH)
        {
            load_db->state = DSP_LOAD_STATE_IDLE;
            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_INIT_FINISH, NULL);
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
            dsp_hal_ready_set(READY_BIT_DSP);
#endif
        }
        else
        {
            load_db->state = DSP_LOAD_STATE_IDLE;

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_DSP_LOAD_FINISH, (void *)scenario);

        }
    }
}

bool dsp_load_is_busy(void)
{
    return (load_db->state == DSP_LOAD_STATE_IDLE ? false : true);
}

bool dsp_load_in_initial(void)
{
    if ((load_db->state == DSP_LOAD_STATE_SYS) || (load_db->state == DSP_LOAD_STATE_PATCH))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void dsp_load_initial(void)
{
    load_db->dsp_load_session = bin_loader_session_create(dsp_anc_fw_load_code_driver,
                                                          dsp_fw_cb_check_finish,
                                                          dsp_fw_cb_finish);

    DIPC_PRINT_TRACE2("dsp_load_initial: dsp_load_session %p, loader_scenario %d",
                      load_db->dsp_load_session, load_db->loader_scenario);

    if (load_db->dsp_load_session != NULL)
    {
        bin_loader_token_issue(load_db->dsp_load_session, load_db->loader_scenario, NULL);
    }

    load_db->state = DSP_LOAD_STATE_SYS;
}

bool dsp_load_need_initial(void)
{
    return (load_db->state == DSP_LOAD_STATE_INIT) ? true : false;
}
