#ifndef _EQ_EXT_FTL_H_
#define _EQ_EXT_FTL_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include "fmc_api.h"

#define EQ_EXT_FTL_PARTITION_NAME "EQ_FTL"

#define AUDIO_AND_APT_EQ_SIZE  (IS_FLASH_SIZE_LARGER_THAN_2MB()? 0x2800 : 0xC00)
#define VOICE_SPK_EQ_SIZE      0x800

void eq_ext_ftl_storage_init(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif
