#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 1)
#include "anc_driver.h"
#include "adsp_loader.h"
#include "trace.h"
#include "os_mem.h"
#include "bin_loader.h"
#include "rtl876x.h"

/* Static define */
#define ANC_SUB_IMAGE_HEADER_MAGIC   0x43215A5A
#define ANC_SUB_IMAGE_HEADER_SIZE    (sizeof(T_DSP_IMAGE))

#define ANC_IMAGE_BUF_SIZE          1024    // for sub image size: 249 words
typedef enum
{
    ADSP_LOAD_STATE_INIT                 = 0x0,
    ADSP_LOAD_STATE_IDLE                 = 0x1,
    ADSP_LOAD_STATE_ALGO                 = 0x2,
} T_ADSP_LOAD_STATE;

typedef struct
{
    uint8_t used_algorithm;
    uint8_t reload_count;
    T_ADSP_LOAD_STATE state;
    T_ADSP_ALGORITHM_SCENARIO loader_scenario;
    T_BIN_LOADER_SESSION_HANDLE adsp_load_session;
} T_ADSP_LOAD_DB;

T_ADSP_LOAD_DB *adsp_load_db = NULL;
/* End of Static define */

/* Extern variable */
extern P_ANC_CBACK anc_loader_cback;

extern bool adsp_fw_load_code_driver(T_BIN_LOADER_SESSION_HANDLE  session,
                                     uint32_t                     id,
                                     void                        *context);
/* End of Extern symbol */

/* Variable define */

/* End of Variable define */
/* TODO Remove End */
#define ADSP_FW_RELOAD_COUNT         5

bool adsp_loader_init(void)
{
    int8_t ret = 0;

    if (adsp_load_db != NULL)
    {
        ret = -1;
        goto fail_check_db;
    }

    adsp_load_db = os_mem_zalloc2(sizeof(T_ADSP_LOAD_DB));
    if (adsp_load_db == NULL)
    {
        ret = -2;
        goto fail_alloc_db;
    }

    adsp_load_db->used_algorithm = ADSP_ALGORITHM_SCENARIO_NONE;

    adsp_load_db->state = ADSP_LOAD_STATE_INIT;

    adsp_load_db->loader_scenario = ADSP_ALGORITHM_SCENARIO_NONE;

    return true;
fail_alloc_db:
fail_check_db:
    DIPC_PRINT_ERROR1("adsp_load_init: fail, ret %d", ret);
    return false;
}

void adsp_load_deinit(void)
{
    if (adsp_load_db != NULL)
    {
        bin_loader_session_destory(adsp_load_db->adsp_load_session);
        os_mem_free(adsp_load_db);
        adsp_load_db = NULL;
    }
}

bool adsp_loader_dsp_fw_cb_check_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                                        uint32_t                     id,
                                        void                        *context)
{
    T_ADSP_ALGORITHM_SCENARIO scenario = adsp_load_get_scenario();

    if (adsp_loader_driver_check_image_correct(scenario) == 0)
    {
        DIPC_PRINT_ERROR1("adsp_loader_dsp_fw_cb_check_finish: ADSP load fail 0x%02X",
                          scenario);
        //Reload algorithm FW
        if ((scenario < ADSP_ALGORITHM_SCENARIO_NONE) &&
            (adsp_load_db->reload_count <= ADSP_FW_RELOAD_COUNT))
        {
            adsp_load_db->reload_count++;
            anc_drv_enable_adaptive_anc(0, 0);
            anc_drv_turn_on_adsp(0);
            adsp_load_algorithm_code(scenario);
        }
    }
    else
    {
        adsp_load_db->reload_count = 0;
    }
    return true;
}

bool adsp_loader_dsp_fw_cb_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                                  T_BIN_LOADER_EVENT           event,
                                  void                        *context)
{
    T_ADSP_ALGORITHM_SCENARIO scenario = adsp_load_get_scenario();

    if (scenario == ADSP_ALGORITHM_SCENARIO_APT)
    {
        DIPC_PRINT_TRACE0("adsp_loader_dsp_fw_cb_finish: ADSP_ALGORITHM_SCENARIO_APT load finished");
    }
    if (scenario == ADSP_ALGORITHM_SCENARIO_ADAPTIVE_ANC)
    {
        DIPC_PRINT_TRACE0("adsp_loader_dsp_fw_cb_finish: ADSP_ALGORITHM_SCENARIO_ADAPTIVE_ANC load finished");
    }

    adsp_load_db->state = ADSP_LOAD_STATE_IDLE;
    anc_drv_set_adsp_load_finished(1);
    if (anc_drv_get_adsp_para_source() == ANC_DRV_ADSP_PARA_SRC_FROM_TOOL)
    {
        anc_drv_turn_on_adsp(1);
    }
    anc_loader_cback(ANC_CB_EVENT_LOAD_SCENARIO_COMPLETE);
    return true;
}

void adsp_load_set_scenario(T_ADSP_ALGORITHM_SCENARIO scenario)
{
    adsp_load_db->loader_scenario = scenario;
}

T_ADSP_ALGORITHM_SCENARIO adsp_load_get_scenario(void)
{
    return adsp_load_db->loader_scenario;
}


bool adsp_load_algorithm_code(uint8_t algorithm)
{
    DIPC_PRINT_TRACE1("adsp_load_algorithm_code: algorithm = 0x%x", algorithm);
    anc_drv_set_adsp_load_finished(0);
    adsp_load_db->loader_scenario = (T_ADSP_ALGORITHM_SCENARIO)algorithm;
    bin_loader_token_issue(adsp_load_db->adsp_load_session, adsp_load_db->loader_scenario, NULL);
    adsp_load_db->state = ADSP_LOAD_STATE_ALGO;

    return true;
}

bool adsp_load_is_busy(void)
{
    return (adsp_load_db->state == ADSP_LOAD_STATE_IDLE ? false : true);
}

bool adsp_load_in_initial(void)
{
    return false;
}

bool adsp_load_initial(void)
{
    int8_t ret = 0;
    adsp_load_db->adsp_load_session = bin_loader_session_create(adsp_fw_load_code_driver,
                                                                adsp_loader_dsp_fw_cb_check_finish,
                                                                adsp_loader_dsp_fw_cb_finish);

    DIPC_PRINT_TRACE2("adsp_load_initial: adsp_load_session %p, loader_scenario %d",
                      adsp_load_db->adsp_load_session, adsp_load_db->loader_scenario);

    if (adsp_load_db->adsp_load_session == NULL)
    {
        ret = -1;
        goto fail_alloc_adsp_load_session;
    }

    adsp_load_db->state = ADSP_LOAD_STATE_IDLE;

    return true;
fail_alloc_adsp_load_session:
    CODEC_PRINT_ERROR1("adsp_load_initial: fail, ret = %d", ret);
    return ret;
}

bool adsp_load_need_initial(void)
{
    return (adsp_load_db->state == ADSP_LOAD_STATE_INIT) ? true : false;
}
#endif

