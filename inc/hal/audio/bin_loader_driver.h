/*============================================================================*
 *              Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __BIN_LOADER_DRIVER_H
#define __BIN_LOADER_DRIVER_H

/*============================================================================*
 *              Header Files
 *============================================================================*/
#include <stdint.h>
#include <bin_loader.h>

#define FPGA_DSP_ROM_TEST
#define DEBUG_DSP_IMAGE_LOAD     0

#ifdef _IS_ASIC_
#undef FPGA_DSP_ROM_TEST
#endif

#define DEBUG_DSP_IMAGE_LOAD_HAL     1//0
#define USE_LOAD_FUNC_IN_ROM         0

#define BIN_LOADER_FLASH_DMA_BUFFER_SIZE   2048

/*============================================================================*
 *              Macros
 *============================================================================*/
#define DSP_FLASH_DMA_BUFFER_SIZE   2048

#define MAGIC_PATTERN               0x43215A5A
#define MAGIC_PATTERN_END           0x1234A5A5

#define SIGNATURE_FAKE_DSP_ROM      0x1234

/*============================================================================*
 *              Types
 *============================================================================*/
typedef struct
{
    uint32_t   magic_pattern;
    uint8_t    type;
    uint8_t    image_size_minus_ofs;  // real size = image_size - image_size_ofs
    uint8_t    rsv8[2];
    uint16_t   checksum;        // if 0, the crc16 is ignored
    uint16_t   encrypt;         // if not 0, the image is encrypted

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
            uint32_t apt_brightness:    1;  /* bit[22] apt_brightness */
            uint32_t rsvd_group_info:   9;  /* bit[31:23] reserved */
        };
    };

    uint32_t   image_size;      // include dummy for 16 bytes for AES block
} T_DSP_IMAGE; // 20 bytes

typedef enum
{
    FROM_SYS,
    FROM_SCENARIO,
#ifdef FPGA_DSP_ROM_TEST
    FROM_ROM,
#endif
    FROM_ANC,
    FROM_ADSP,
    FROM_DSP2_SYS,
    FROM_INVALID,
} T_IMAGE_SRC;

typedef enum
{
    // up 200 will put into dsp_sys_image.bin
    SHM_SCENARIO_SYS_ROM = 255, // only fpga
    SHM_SCENARIO_SYS_RAM = 254,
    SHM_SCENARIO_SYS_PATCH = 253,
    SHM_SCENARIO_DSP2_SYS_1 = 252,
    SHM_SCENARIO_DSP2_SYS_2 = 251,
    SHM_SCENARIO_DSP2_SYS_3 = 250,

    // 128 ~ 135 will put into anc image
    SHM_SCENARIO_ANC_0_CFG  = 128,
    SHM_SCENARIO_ANC_1_COEF = 129,
    SHM_SCENARIO_ANC_2_COEF = 130,
    SHM_SCENARIO_ANC_3_COEF = 131,
    SHM_SCENARIO_ANC_4_COEF = 132,
    SHM_SCENARIO_ANC_5_COEF = 133,
    SHM_SCENARIO_ANC_EAR_FIT = 134,
    SHM_SCENARIO_ANC_APT_COEF = 135,
    SHM_SCENARIO_ANC_ADSP_PARA = 136,
    SHM_SCENARIO_ANC_APT_FILTER_INFO = 139,

    // up 200 will put into dsp_sys_image.bin
    // below 200 will put into dsp_scenario_image.bin
    SHM_SCENARIO_A2DP = 0,
    SHM_SCENARIO_SCO = 1,
    SHM_SCENARIO_VOICE_RECOGNITION = 2,
    SHM_SCENARIO_PROPRIETARY_VOICE = 3,
    SHM_SCENARIO_OPUS_VOICE = 4,
    SHM_SCENARIO_UHQ_VOICE = 5,
    SHM_SCENARIO_PROPRIETARY_AUDIO1 = 6,
    SHM_SCENARIO_PROPRIETARY_AUDIO2 = 7,
    SHM_SCENARIO_MP3 = 8,
    SHM_SCENARIO_LDAC = 9,
    SHM_SCENARIO_UHQ = 10,
    SHM_SCENARIO_LHDC = 11,
    SHM_SCENARIO_OPUS_AUDIO = 12,
    SHM_SCENARIO_PROPRIETARY_AUDIO3 = 13,
    SHM_SCENARIO_PROPRIETARY_AUDIO4 = 14,
    SHM_SCENARIO_ALL = 15,

    SHM_SCANRRIO_TOTAL
} T_SHM_SCENARIO;

typedef enum
{
    SHM_SCENARIO_ADSP_APT = 0,
    SHM_SCENARIO_ADSP_ADAPTIVE_ANC = 1,
    SHM_SCANRRIO_ADSP_NONE
} T_ADSP_SCENARIO;

/*============================================================================*
 *              Variables
 *============================================================================*/
typedef void (*BIN_LOADER_DRIVER_CB)(void);
typedef void (*DSP_USER_PARSER_CB)(uint8_t type);

void dsp_fw_dma_handler(void);
void bin_loader_driver_load_next_bin(uint8_t type);
uint8_t bin_loader_driver_check_image_correct(uint8_t type);
uint8_t adsp_loader_driver_check_image_correct(uint8_t type);
void bin_loader_driver_reg_cb(BIN_LOADER_DRIVER_CB check_finish, BIN_LOADER_DRIVER_CB finish,
                              BIN_LOADER_DRIVER_CB err);
bool dsp_anc_fw_load_code_driver(T_BIN_LOADER_SESSION_HANDLE  session,
                                 uint32_t                     id,
                                 void                        *context);

#endif // end of __BIN_LOADER_DRIVER_H
