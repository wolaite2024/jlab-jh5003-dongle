#if (CONFIG_REALTEK_AM_ANC_SUPPORT == 1)
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "trace.h"
#include "os_mem.h"
#include "rtl876x.h"
#include "bin_loader.h"
#include "anc_driver.h"
#include "anc_loader.h"
#include "adsp_loader.h"

/* Static define */
#define ANC_SUB_IMAGE_HEADER_MAGIC   0x43215A5A
#define ANC_SUB_IMAGE_HEADER_SIZE    (sizeof(T_DSP_IMAGE))

#define ANC_IMAGE_BUF_SIZE          1024    // for sub image size: 249 words
/* End of Static define */

/* Extern variable */
extern T_DSP_IMAGE_EXT *p_dsp_image;
extern uint32_t *p_image_buffer;
extern T_ADSP_ALGORITHM_SCENARIO adsp_pre_scenario_from_bin;

/* End of Extern symbol */

/* Variable define */
P_ANC_CBACK anc_loader_cback = NULL;

/* End of Variable define */

bool anc_loader_init(P_ANC_CBACK cback)
{
    anc_loader_cback = cback;

    return true;
}

bool anc_loader_dsp_fw_cb_check_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                                       uint32_t                     id,
                                       void                        *context)
{
    // dummy callback, do nothing
    return true;
}

bool anc_loader_dsp_fw_cb_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                                 T_BIN_LOADER_EVENT           event,
                                 void                        *context)
{
    uint32_t image_size = p_dsp_image->image_size - p_dsp_image->image_size_minus_ofs;

    CODEC_PRINT_TRACE6("anc_loader_dsp_fw_cb_finish, payload: %p, p_dsp_image = 0x%x, type: 0x%X, grp_info: 0x%X, image_size: %d, anc_image: %p",
                       p_image_buffer, p_dsp_image, p_dsp_image->type, p_dsp_image->group_info, image_size, p_dsp_image);
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 1)
    if (((p_dsp_image->type >= SHM_SCENARIO_ANC_0_CFG) &&
         (p_dsp_image->type <= SHM_SCENARIO_ANC_ADSP_PARA)) ||
        (p_dsp_image->type == SHM_SCENARIO_ANC_APT_FILTER_INFO))
#else
    if (((p_dsp_image->type >= SHM_SCENARIO_ANC_0_CFG) &&
         (p_dsp_image->type <= SHM_SCENARIO_ANC_APT_COEF)) ||
        (p_dsp_image->type == SHM_SCENARIO_ANC_APT_FILTER_INFO))
#endif
    {
        // parsing ANC image
        if (p_image_buffer)
        {
            anc_drv_image_parser(p_dsp_image, p_image_buffer);
            free(p_image_buffer);
            p_image_buffer = NULL;

            if (p_dsp_image->type == SHM_SCENARIO_ANC_0_CFG)
            {
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 1)
                anc_drv_adaptive_anc_boot_up();
#endif
                anc_loader_cback(ANC_CB_EVENT_LOAD_CONFIGURATION_COMPLETE);
            }
            else if (p_dsp_image->type == SHM_SCENARIO_ANC_APT_COEF)
            {
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 1)
                T_ADSP_ALGORITHM_SCENARIO adsp_curr_scenario_from_bin = ADSP_ALGORITHM_SCENARIO_NONE;
                uint32_t target_gain = anc_drv_get_active_adsp_scenario();
                if (target_gain & BIT14)
                {
                    adsp_curr_scenario_from_bin = ADSP_ALGORITHM_SCENARIO_APT;
                }
                else if (target_gain & BIT15)
                {
                    adsp_curr_scenario_from_bin = ADSP_ALGORITHM_SCENARIO_APT;
                }
                else if (target_gain & BIT16)
                {
                    adsp_curr_scenario_from_bin = ADSP_ALGORITHM_SCENARIO_ADAPTIVE_ANC;
                }

                if (((target_gain & BIT14) || (target_gain & BIT15) || (target_gain & BIT16)) &&
                    (adsp_curr_scenario_from_bin != adsp_pre_scenario_from_bin))
                {
                    adsp_load_algorithm_code(adsp_curr_scenario_from_bin);
                    adsp_pre_scenario_from_bin = adsp_curr_scenario_from_bin;
                    anc_drv_set_adsp_para_source(ANC_DRV_ADSP_PARA_SRC_FROM_BIN);
                    anc_loader_cback(ANC_CB_EVENT_LOAD_ADSP_IMAGE);
                }
                else
                {
                    anc_loader_cback(ANC_CB_EVENT_LOAD_SCENARIO_COMPLETE);
                }
#else
                anc_loader_cback(ANC_CB_EVENT_LOAD_SCENARIO_COMPLETE);
#endif
            }
#if (CONFIG_REALTEK_AM_ADSP_SUPPORT == 1)
            else if (p_dsp_image->type == SHM_SCENARIO_ANC_ADSP_PARA)
            {
                anc_loader_cback(ANC_CB_EVENT_LOAD_ADSP_PARA_COMPLETE);
            }
#endif
            else if (p_dsp_image->type == SHM_SCENARIO_ANC_APT_FILTER_INFO)
            {
                anc_loader_cback(ANC_CB_EVENT_FILTER_INFO_COMPLETE);
            }
        }
    }

    return true;
}

void anc_loader_set_img_load_param(uint8_t sub_type, uint8_t scenario_id)
{
    anc_drv_set_img_load_param(sub_type, scenario_id);
}
#endif

