#ifndef _ANC_LOADER_H_
#define _ANC_LOADER_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "bin_loader.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

// ref. to T_DSP_IMAGE, 20 bytes
typedef struct t_dsp_image_ext
{
    uint32_t magic_pattern;
    uint8_t type;
    uint8_t image_size_minus_ofs;   // real size = image_size - image_size_ofs
    uint8_t rsv8[2];
    uint16_t checksum;              // if 0, the crc16 is ignored
    uint16_t encrypt;               // if not 0, the image is encrypted

    union
    {
        uint32_t image_dst_addr;    // DSP image usage
        uint32_t group_info;        // ANC image usage
        struct
        {
            /* bit[0]: ANC, bit[1]: idle APT, bit[2]: A2DP APT, bit[3]: SCO APT */
            uint32_t sub_type:          4;  /* bit[3:0] filtergrouptype */
            uint32_t anc_apt_scenario:  8;  /* bit[11:4] scenario id */
            uint32_t scenario_mode:     8;  /* bit[19:12] enumerate for phone APP */
            uint32_t l_result_for_fit:  1;  /* bit[20] L channel response for ear tips fit */
            uint32_t r_result_for_fit:  1;  /* bit[21] R channel response for ear tips fit */
            uint32_t rsvd_group_info:   10; /* bit[31:22] reserved */
        };
    };

    uint32_t image_size;            // include dummy for 16 bytes for AES block
} T_DSP_IMAGE_EXT;

bool anc_loader_init(P_ANC_CBACK cback);

bool anc_loader_dsp_fw_cb_check_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                                       uint32_t                     id,
                                       void                        *context);
bool anc_loader_dsp_fw_cb_finish(T_BIN_LOADER_SESSION_HANDLE  session,
                                 T_BIN_LOADER_EVENT           event,
                                 void                        *context);
void anc_loader_set_img_load_param(uint8_t sub_type, uint8_t scenario_id);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif

