/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */


#ifndef _DSP_LOADER_H_
#define _DSP_LOADER_H_

#include <stdint.h>

#include "bin_loader_driver.h"
#include "trace.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

bool dsp_load_init(void);
void dsp_load_deinit(void);
bool dsp_load_algorithm_code(T_DSP_SESSION_TYPE type);
bool dsp_loader_bin_match(T_DSP_SESSION_TYPE type);
void dsp_load_set_scenario(T_SHM_SCENARIO scenario);
T_SHM_SCENARIO dsp_load_get_scenario(void);
void dsp_load_next_bin(void);
void dsp_load_finish(void);
bool dsp_load_is_busy(void);
bool dsp_load_need_initial(void);
void dsp_load_initial(void);
bool dsp_load_in_initial(void);
void dsp_load_set_test_bin(bool enable);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* _DSP_LOADER_H_ */

