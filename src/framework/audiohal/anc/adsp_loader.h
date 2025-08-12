/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _ADSP_LOADER_H_
#define _ADSP_LOADER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    ADSP_ALGORITHM_SCENARIO_APT = 0,
    ADSP_ALGORITHM_SCENARIO_ADAPTIVE_ANC = 1,
    ADSP_ALGORITHM_SCENARIO_NONE = 2,
} T_ADSP_ALGORITHM_SCENARIO;

bool adsp_loader_init(void);
void adsp_load_deinit(void);
bool adsp_load_algorithm_code(uint8_t decode_algorithm);
void adsp_load_set_scenario(T_ADSP_ALGORITHM_SCENARIO scenario);
T_ADSP_ALGORITHM_SCENARIO adsp_load_get_scenario(void);
void adsp_load_next_bin(void);
void adsp_load_finish(void);
bool adsp_load_is_busy(void);
bool adsp_load_need_initial(void);
bool adsp_load_initial(void);
bool adsp_load_in_initial(void);
extern uint8_t *p_adsp_para_buf;
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* _ADSP_LOADER_H_ */

