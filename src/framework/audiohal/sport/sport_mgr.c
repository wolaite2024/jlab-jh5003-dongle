/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "sport_driver.h"
#include "rtl876x_pinmux.h"
#include "sys_cfg.h"
#include "audio_route.h"
#include "sport_mgr.h"

#define SPORT0_HANDLE               (0)
#define SPORT1_HANDLE               (1)
#define SPORT2_HANDLE               (2)
#define SPORT3_HANDLE               (3)
#define SPORT_NUMBER                (4)

#define SPORT_PATH_NONE             (0x00)
#define SPORT_PATH_DA               (0x01)
#define SPORT_PATH_AD               (0x02)

typedef enum t_sport_mgr_chann_mode
{
    SPORT_MGR_CHANN_MODE_STEREO,
    SPORT_MGR_CHANN_MODE_MONO,
} T_SPORT_MGR_CHANN_MODE;

typedef struct t_sport_mgr_cfg
{
    uint8_t                 id;
    uint8_t                 rtx;
    T_SPORT_MGR_CHANN_MODE  rx_chann_mode;
    T_SPORT_MGR_CHANN_MODE  tx_chann_mode;
    uint32_t                rx_sample_rate;
    uint32_t                tx_sample_rate;
} T_SPORT_MGR_CFG;

typedef struct t_sport_pin_func
{
    uint8_t lrc_pin_func;
    uint8_t bclk_pin_func;
    uint8_t adcdat_pin_func;
    uint8_t dacdat_pin_func;
} T_SPORT_PIN_FUNC;

typedef struct t_sport_path_db
{
    uint32_t                sample_rate;
    T_SPORT_CHANNEL_MODE    channel_mode;
    uint8_t                 ref;
    bool                    bridge;
} T_SPORT_PATH_DB;

typedef struct t_sport_db
{
    T_SPORT_PATH_DB     rx;
    T_SPORT_PATH_DB     tx;
} T_SPORT_DB;

typedef struct t_sport_mgr_session
{
    T_OS_QUEUE_ELEM         element;
    T_AUDIO_CATEGORY        category;
    uint8_t                 path_num;
    T_SPORT_MGR_CFG        *param;
} T_SPORT_MGR_SESSION;

typedef struct t_sport_init_cfg
{
    uint8_t sport_idx                         : 4;
    uint8_t sport_enable                      : 1;
    uint8_t sport_role                        : 1;
    uint8_t sport_tx_bridge                   : 1;
    uint8_t sport_rx_bridge                   : 1;

    uint32_t sport_lrc_pinmux                  : 8;
    uint32_t sport_bclk_pinmux                 : 8;
    uint32_t sport_adc_dat_pinmux              : 8;
    uint32_t sport_dac_dat_pinmux              : 8;

    uint32_t sport_ext_mclk_enable              : 1;
    uint32_t sport_ext_mclk_rate                : 4;
    uint32_t sport_ext_mclk_pinmux              : 8;
    uint32_t sport_ext_rsvd                     : 19;
} T_SPORT_INIT_CFG;

typedef struct t_sport_path_group
{
    uint8_t  sport_path_number;
    uint8_t *sport_id;
    uint8_t *sport_direction;
} T_SPORT_PATH_GROUP;

typedef struct t_sport_mgr_db
{
    T_OS_QUEUE              session_list;
    T_SPORT_DB              sport[AUDIO_ROUTE_SPORT_NUM];
} T_SPORT_MGR_DB;

static T_SPORT_MGR_DB sport_mgr_db;

static const T_SPORT_PIN_FUNC pin_func_map[AUDIO_ROUTE_SPORT_NUM] =
{
    [AUDIO_ROUTE_SPORT_0] = {LRC_SPORT0, BCLK_SPORT0, ADCDAT_SPORT0, DACDAT_SPORT0},
    [AUDIO_ROUTE_SPORT_1] = {LRC_SPORT1, BCLK_SPORT1, ADCDAT_SPORT1, DACDAT_SPORT1},
};

static const T_SPORT_CHANNEL_LEN channel_len_map[] =
{
    [AUDIO_ROUTE_SPORT_CH_LEN_8_BIT]  = {SPORT_CHANNEL_LEN_8_BIT},
    [AUDIO_ROUTE_SPORT_CH_LEN_16_BIT] = {SPORT_CHANNEL_LEN_16_BIT},
    [AUDIO_ROUTE_SPORT_CH_LEN_20_BIT] = {SPORT_CHANNEL_LEN_20_BIT},
    [AUDIO_ROUTE_SPORT_CH_LEN_24_BIT] = {SPORT_CHANNEL_LEN_24_BIT},
    [AUDIO_ROUTE_SPORT_CH_LEN_32_BIT] = {SPORT_CHANNEL_LEN_32_BIT},
};

static const T_SPORT_DATA_LEN data_len_map[] =
{
    [AUDIO_ROUTE_SPORT_DATA_LEN_8_BIT]  = {SPORT_DATA_LEN_8_BIT},
    [AUDIO_ROUTE_SPORT_DATA_LEN_16_BIT] = {SPORT_DATA_LEN_16_BIT},
    [AUDIO_ROUTE_SPORT_DATA_LEN_20_BIT] = {SPORT_DATA_LEN_20_BIT},
    [AUDIO_ROUTE_SPORT_DATA_LEN_24_BIT] = {SPORT_DATA_LEN_24_BIT},
    [AUDIO_ROUTE_SPORT_DATA_LEN_32_BIT] = {SPORT_DATA_LEN_32_BIT},
};

static const float mclk_rate_map[] =
{
    [AUDIO_ROUTE_MCLK_RATE_1P024MHZ]   = 1024.0f,
    [AUDIO_ROUTE_MCLK_RATE_1P4112MHZ]  = 1411.2f,
    [AUDIO_ROUTE_MCLK_RATE_2P048MHZ]   = 2048.0f,
    [AUDIO_ROUTE_MCLK_RATE_2P8224MHZ]  = 2822.4f,
    [AUDIO_ROUTE_MCLK_RATE_3P072MHZ]   = 3072.0f,
    [AUDIO_ROUTE_MCLK_RATE_4P096MHZ]   = 4096.0f,
    [AUDIO_ROUTE_MCLK_RATE_5P6448MHZ]  = 5644.8f,
    [AUDIO_ROUTE_MCLK_RATE_6P144MHZ]   = 6144.0f,
    [AUDIO_ROUTE_MCLK_RATE_8P192MHZ]   = 8192.0f,
    [AUDIO_ROUTE_MCLK_RATE_11P2896MHZ] = 11289.6f,
    [AUDIO_ROUTE_MCLK_RATE_12P288MHZ]  = 12288.0f,
    [AUDIO_ROUTE_MCLK_RATE_16P384MHZ]  = 16384.0f,
    [AUDIO_ROUTE_MCLK_RATE_22P5792MHZ] = 22579.2f,
    [AUDIO_ROUTE_MCLK_RATE_24P576MHZ]  = 24576.0f,
    [AUDIO_ROUTE_MCLK_RATE_32P768MHZ]  = 32768.0f,
};

static T_SPORT_PATH_DB *sport_mgr_path_db_get(uint32_t sport_handle, uint32_t sport_path)
{
    T_SPORT_PATH_DB *sport_path_db;

    sport_path_db = NULL;

    if (sport_path == SPORT_PATH_AD)
    {
        sport_path_db = &sport_mgr_db.sport[sport_handle].rx;
    }

    if (sport_path == SPORT_PATH_DA)
    {
        sport_path_db = &sport_mgr_db.sport[sport_handle].tx;
    }

    return sport_path_db;
}

uint32_t sport_mgr_sample_rate_get(uint32_t sport_handle, uint32_t sport_path)
{
    T_SPORT_PATH_DB *sport_path_db;
    uint32_t sample_rate;

    sample_rate = 0;
    sport_path_db = sport_mgr_path_db_get(sport_handle, sport_path);

    if (sport_path_db != NULL)
    {
        sample_rate = sport_path_db->sample_rate;
    }

    return sample_rate;
}

uint32_t sport_mgr_tx_sample_rate_get(T_SPORT_MGR_SESSION_HANDLE handle)
{
    uint32_t sport_handle;
    uint32_t sport_path;
    T_SPORT_MGR_SESSION *session;
    T_SPORT_PATH_DB *sport_path_db;
    uint32_t sample_rate = 0;

    session = (T_SPORT_MGR_SESSION *)handle;

    for (uint8_t i = 0; i < session->path_num; ++i)
    {
        sport_handle = session->param[i].id;
        sport_path   = session->param[i].rtx;

        if (sport_path & SPORT_PATH_DA)
        {
            sport_path_db = sport_mgr_path_db_get(sport_handle, SPORT_PATH_DA);
            if (sport_path_db != NULL)
            {
                sample_rate = sport_path_db->sample_rate;
                break;
            }
        }
    }

    return sample_rate;
}

uint32_t sport_mgr_rx_sample_rate_get(T_SPORT_MGR_SESSION_HANDLE handle)
{
    uint32_t sport_handle;
    uint32_t sport_path;
    T_SPORT_MGR_SESSION *session;
    T_SPORT_PATH_DB *sport_path_db;
    uint32_t sample_rate = 0;

    session = (T_SPORT_MGR_SESSION *)handle;

    for (uint8_t i = 0; i < session->path_num; ++i)
    {
        sport_handle = session->param[i].id;
        sport_path   = session->param[i].rtx;

        if (sport_path & SPORT_PATH_AD)
        {
            sport_path_db = sport_mgr_path_db_get(sport_handle, SPORT_PATH_AD);
            if (sport_path_db != NULL)
            {
                sample_rate = sport_path_db->sample_rate;
                break;
            }
        }
    }

    return sample_rate;
}

T_SPORT_INIT_CFG sport_mgr_init_cfg_get(uint32_t sport_handle)
{
    T_SPORT_INIT_CFG sport_cfg;

    memset(&sport_cfg, 0, sizeof(T_SPORT_INIT_CFG));

    switch (sport_handle)
    {
    case SPORT0_HANDLE:
        {
            sport_cfg.sport_idx = (uint8_t)sport_handle;
            sport_cfg.sport_enable = sys_cfg_const.sport0_enable;
            sport_cfg.sport_role = sys_cfg_const.sport0_role;
            sport_cfg.sport_tx_bridge = sys_cfg_const.sport0_tx_bridge;
            sport_cfg.sport_rx_bridge = sys_cfg_const.sport0_rx_bridge;
            sport_cfg.sport_lrc_pinmux = sys_cfg_const.sport0_lrc_pinmux;
            sport_cfg.sport_bclk_pinmux = sys_cfg_const.sport0_bclk_pinmux;
            sport_cfg.sport_adc_dat_pinmux = sys_cfg_const.sport0_adc_dat_pinmux;
            sport_cfg.sport_dac_dat_pinmux = sys_cfg_const.sport0_dac_dat_pinmux;
            sport_cfg.sport_ext_mclk_enable = sys_cfg_const.sport_ext_mclk_enable;
            sport_cfg.sport_ext_mclk_pinmux = sys_cfg_const.sport_ext_mclk_pinmux;
            sport_cfg.sport_ext_mclk_rate = sys_cfg_const.sport_ext_mclk_rate;
        }
        break;

    case SPORT1_HANDLE:
        {
            sport_cfg.sport_idx = (uint8_t)sport_handle;
            sport_cfg.sport_enable = sys_cfg_const.sport1_enable;
            sport_cfg.sport_role = sys_cfg_const.sport1_role;
            sport_cfg.sport_tx_bridge = sys_cfg_const.sport1_tx_bridge;
            sport_cfg.sport_rx_bridge = sys_cfg_const.sport1_rx_bridge;
            sport_cfg.sport_lrc_pinmux = sys_cfg_const.sport1_lrc_pinmux;
            sport_cfg.sport_bclk_pinmux = sys_cfg_const.sport1_bclk_pinmux;
            sport_cfg.sport_adc_dat_pinmux = sys_cfg_const.sport1_adc_dat_pinmux;
            sport_cfg.sport_dac_dat_pinmux = sys_cfg_const.sport1_dac_dat_pinmux;
            sport_cfg.sport_ext_mclk_enable = sys_cfg_const.sport_ext_mclk_enable;
            sport_cfg.sport_ext_mclk_pinmux = sys_cfg_const.sport_ext_mclk_pinmux;
            sport_cfg.sport_ext_mclk_rate = sys_cfg_const.sport_ext_mclk_rate;
        }
        break;

    case SPORT2_HANDLE:
        {
            sport_cfg.sport_idx = (uint8_t)sport_handle;
            sport_cfg.sport_enable = sys_cfg_const.sport2_enable;
            sport_cfg.sport_role = sys_cfg_const.sport2_role;
            sport_cfg.sport_tx_bridge = sys_cfg_const.sport2_tx_bridge;
            sport_cfg.sport_rx_bridge = sys_cfg_const.sport2_rx_bridge;
            sport_cfg.sport_lrc_pinmux = sys_cfg_const.sport2_lrc_pinmux;
            sport_cfg.sport_bclk_pinmux = sys_cfg_const.sport2_bclk_pinmux;
            sport_cfg.sport_adc_dat_pinmux = sys_cfg_const.sport2_adc_dat_pinmux;
            sport_cfg.sport_dac_dat_pinmux = sys_cfg_const.sport2_dac_dat_pinmux;
            sport_cfg.sport_ext_mclk_enable = sys_cfg_const.sport_ext_mclk_enable;
            sport_cfg.sport_ext_mclk_pinmux = sys_cfg_const.sport_ext_mclk_pinmux;
            sport_cfg.sport_ext_mclk_rate = sys_cfg_const.sport_ext_mclk_rate;
        }
        break;

    case SPORT3_HANDLE:
        {
            sport_cfg.sport_idx = (uint8_t)sport_handle;
            sport_cfg.sport_enable = sys_cfg_const.sport3_enable;
            sport_cfg.sport_role = sys_cfg_const.sport3_role;
            sport_cfg.sport_tx_bridge = sys_cfg_const.sport3_tx_bridge;
            sport_cfg.sport_rx_bridge = sys_cfg_const.sport3_rx_bridge;
            sport_cfg.sport_lrc_pinmux = sys_cfg_const.sport3_lrc_pinmux;
            sport_cfg.sport_bclk_pinmux = sys_cfg_const.sport3_bclk_pinmux;
            sport_cfg.sport_adc_dat_pinmux = sys_cfg_const.sport3_adc_dat_pinmux;
            sport_cfg.sport_dac_dat_pinmux = sys_cfg_const.sport3_dac_dat_pinmux;
            sport_cfg.sport_ext_mclk_enable = sys_cfg_const.sport_ext_mclk_enable;
            sport_cfg.sport_ext_mclk_pinmux = sys_cfg_const.sport_ext_mclk_pinmux;
            sport_cfg.sport_ext_mclk_rate = sys_cfg_const.sport_ext_mclk_rate;
        }
        break;

    default:
        {

        }
        break;
    }

    return sport_cfg;
}

T_SPORT_PATH_GROUP sport_mgr_sport_path_take(T_AUDIO_CATEGORY category)
{
    uint8_t tx_sel[SPORT_NUMBER]    = {0};
    uint8_t rx_sel[SPORT_NUMBER]    = {0};
    uint8_t sport_id[SPORT_NUMBER]  = {0};
    uint8_t sport_direction[SPORT_NUMBER] = {0};
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP pysical_path_group;
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    uint8_t physical_path_num;

    T_SPORT_PATH_GROUP sport_path_group =
    {
        .sport_path_number = 0,
        .sport_id = NULL,
        .sport_direction = NULL,
    };

    pysical_path_group = audio_route_physical_path_take(category);
    physical_path      = pysical_path_group.physical_path;
    physical_path_num  = pysical_path_group.physical_path_num;

    if (physical_path != NULL)
    {
        for (uint8_t i = 0; i < physical_path_num; i++)
        {
            if ((category == AUDIO_CATEGORY_APT) &&
                ((physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER) || \
                 (physical_path->logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER)))
            {
                physical_path++;
                continue;
            }
            else
            {
                if (physical_path->sport_ch_dir == AUDIO_ROUTE_SPORT_CH_DIR_TX)
                {
                    tx_sel[physical_path->sport_idx] = 1;
                }
                else
                {
                    rx_sel[physical_path->sport_idx] = 1;
                }
                physical_path++;
            }
        }

        for (uint8_t i = 0; i < SPORT_NUMBER; i++)
        {
            if ((tx_sel[i] == 1) || (rx_sel[i] == 1))
            {
                sport_id[sport_path_group.sport_path_number] = i;

                sport_direction[sport_path_group.sport_path_number] = SPORT_PATH_NONE;

                if (tx_sel[i] == 1)
                {
                    sport_direction[sport_path_group.sport_path_number] |= SPORT_PATH_DA;
                }

                if (rx_sel[i] == 1)
                {
                    sport_direction[sport_path_group.sport_path_number] |= SPORT_PATH_AD;
                }

                sport_path_group.sport_path_number++;
            }
        }

        if (sport_path_group.sport_path_number != 0)
        {
            sport_path_group.sport_direction = os_mem_zalloc2(sport_path_group.sport_path_number);
            sport_path_group.sport_id        = os_mem_zalloc2(sport_path_group.sport_path_number);
            if ((sport_path_group.sport_direction != NULL) && (sport_path_group.sport_id != NULL))
            {
                memcpy(sport_path_group.sport_id, sport_id, sport_path_group.sport_path_number);
                memcpy(sport_path_group.sport_direction, sport_direction, sport_path_group.sport_path_number);
            }
        }
        audio_route_physical_path_give(&pysical_path_group);
    }
    return sport_path_group;
}

void sport_mgr_sport_path_give(T_SPORT_PATH_GROUP *sport_path_group)
{
    if (sport_path_group->sport_path_number != 0)
    {
        if (sport_path_group->sport_direction != NULL)
        {
            os_mem_free(sport_path_group->sport_direction);
        }

        if (sport_path_group->sport_id != NULL)
        {
            os_mem_free(sport_path_group->sport_id);
        }
    }
}

bool sport_mgr_init(void)
{
    T_SPORT_INIT_CFG sport_cfg;

    for (uint8_t sport_handle = 0; sport_handle < AUDIO_ROUTE_SPORT_NUM; ++sport_handle)
    {
        sport_cfg = sport_mgr_init_cfg_get(sport_handle);
        sport_mgr_db.sport[sport_handle].tx.bridge = sport_cfg.sport_tx_bridge;
        sport_mgr_db.sport[sport_handle].tx.ref = 0;
        sport_mgr_db.sport[sport_handle].tx.sample_rate = 16000;
        sport_mgr_db.sport[sport_handle].tx.channel_mode = SPORT_MODE_MONO;

        sport_mgr_db.sport[sport_handle].rx.bridge = sport_cfg.sport_rx_bridge;
        sport_mgr_db.sport[sport_handle].rx.ref = 0;
        sport_mgr_db.sport[sport_handle].rx.sample_rate = 16000;
        if (sport_handle == AUDIO_ROUTE_SPORT_1)
        {
            sport_mgr_db.sport[sport_handle].rx.channel_mode = SPORT_MODE_STEREO;
        }
        else
        {
            sport_mgr_db.sport[sport_handle].rx.channel_mode = SPORT_MODE_MONO;
        }
    }

    os_queue_init(&sport_mgr_db.session_list);

    return true;
}

void sport_mgr_deinit(void)
{

}

static void sport_mgr_param_set(uint32_t sport_handle)
{
    T_SPORT_TRX_CFG sport_trx_cfg;
    uint32_t tx_sample_rate;
    uint32_t rx_sample_rate;
    T_SPORT_CHANNEL_MODE tx_channel_mode;
    T_SPORT_CHANNEL_MODE rx_channel_mode;
    T_SPORT_DATA_LEN tx_data_length;
    T_SPORT_DATA_LEN rx_data_length;
    T_SPORT_TDM_MODE tx_tdm_mode;
    T_SPORT_TDM_MODE rx_tdm_mode;
    T_SPORT_CHANNEL_LEN tx_chann_len;
    T_SPORT_CHANNEL_LEN rx_chann_len;
    T_SPORT_FORMAT_TYPE tx_format;
    T_SPORT_FORMAT_TYPE rx_format;
    T_AUDIO_ROUTE_SPORT_CFG tx;
    T_AUDIO_ROUTE_SPORT_CFG rx;
    T_SPORT_PATH_DB *rx_db;
    T_SPORT_PATH_DB *tx_db;

    rx_db           = sport_mgr_path_db_get(sport_handle, SPORT_PATH_AD);
    tx_db           = sport_mgr_path_db_get(sport_handle, SPORT_PATH_DA);
    tx              = audio_route_sport_cfg_get((T_AUDIO_ROUTE_SPORT_IDX)sport_handle,
                                                AUDIO_ROUTE_SPORT_CH_DIR_TX);
    rx              = audio_route_sport_cfg_get((T_AUDIO_ROUTE_SPORT_IDX)sport_handle,
                                                AUDIO_ROUTE_SPORT_CH_DIR_RX);

    tx_sample_rate  = tx_db->sample_rate;
    rx_sample_rate  = rx_db->sample_rate;
    tx_channel_mode = (T_SPORT_CHANNEL_MODE)tx_db->channel_mode;
    rx_channel_mode = SPORT_MODE_STEREO;
    tx_data_length = data_len_map[tx.data_len];
    rx_data_length = data_len_map[rx.data_len];
    tx_tdm_mode    = (T_SPORT_TDM_MODE)tx.mode;
    rx_tdm_mode    = (T_SPORT_TDM_MODE)rx.mode;
    tx_chann_len   = channel_len_map[tx.chann_len];
    rx_chann_len   = channel_len_map[rx.chann_len];
    tx_format = (T_SPORT_FORMAT_TYPE)tx.format;
    rx_format = (T_SPORT_FORMAT_TYPE)rx.format;

    sport_drv_trx_config_default(&sport_trx_cfg);
    sport_trx_cfg.tx_sample_rate = tx_sample_rate;
    sport_trx_cfg.rx_sample_rate = rx_sample_rate;

    sport_trx_cfg.tx_channel_mode = tx_channel_mode;
    sport_trx_cfg.rx_channel_mode = rx_channel_mode;

    sport_trx_cfg.tx_data_length    = tx_data_length;
    sport_trx_cfg.rx_data_length    = rx_data_length;
    sport_trx_cfg.tx_tdm_mode       = tx_tdm_mode;
    sport_trx_cfg.rx_tdm_mode       = rx_tdm_mode;
    sport_trx_cfg.rx_channel_length = rx_chann_len;
    sport_trx_cfg.tx_channel_length = tx_chann_len;

    sport_trx_cfg.tx_format_type = tx_format;
    sport_trx_cfg.rx_format_type = rx_format;

    sport_drv_trx_config_set((T_SPORT_ID)sport_handle, &sport_trx_cfg);
    sport_drv_dump_setting((T_SPORT_ID)sport_handle);

    DIPC_PRINT_TRACE5("sport_mgr_param_set: sport_handle %d, rx_chann_len %d, tx_chann_len %d, tx_format %d, rx_format %d",
                      sport_handle, rx_chann_len, tx_chann_len, tx_format, rx_format);

    DIPC_PRINT_TRACE8("sport_mgr_param_set: tx_sample_rate %d, rx_sample_rate %d, tx_channel_mode %d, "
                      "rx_channel_mode %d, tx_data_length %u, rx_data_length %u, tx_tdm_mode %d, "
                      "rx_tdm_mode %d", tx_sample_rate, rx_sample_rate, tx_channel_mode,
                      rx_channel_mode, tx_data_length, rx_data_length, tx_tdm_mode, rx_tdm_mode);

}

static void sport_mgr_param_init(uint32_t sport_handle)
{
    bool             enable;
    bool             mclk_enable;
    bool             tx_bridge;
    bool             rx_bridge;
    uint8_t          sport_lrc_pinmux;
    uint8_t          sport_bclk_pinmux;
    uint8_t          sport_adc_dat_pinmux;
    uint8_t          sport_dac_dat_pinmux;
    uint8_t          sport_mclk_pinmux;
    T_SPORT_ID       sport_id;
    T_SPORT_ROLE     sport_role;
    T_SPORT_MISC_CFG sport_misc_cfg;
    T_SPORT_INIT_CFG sport_cfg;
    float            freq_mclk;

    sport_cfg = sport_mgr_init_cfg_get(sport_handle);

    sport_id             = (T_SPORT_ID)sport_cfg.sport_idx;
    enable               = (bool)sport_cfg.sport_enable;
    sport_role           = (T_SPORT_ROLE)sport_cfg.sport_role;
    mclk_enable          = (bool)sport_cfg.sport_ext_mclk_enable;
    tx_bridge            = (bool)sport_cfg.sport_tx_bridge;
    rx_bridge            = (bool)sport_cfg.sport_rx_bridge;
    sport_lrc_pinmux     = sport_cfg.sport_lrc_pinmux;
    sport_bclk_pinmux    = sport_cfg.sport_bclk_pinmux;
    sport_adc_dat_pinmux = sport_cfg.sport_adc_dat_pinmux;
    sport_dac_dat_pinmux = sport_cfg.sport_dac_dat_pinmux;
    sport_mclk_pinmux    = sport_cfg.sport_ext_mclk_pinmux;
    freq_mclk            = mclk_rate_map[sport_cfg.sport_ext_mclk_rate];

    if (sport_cfg.sport_tx_bridge || sport_cfg.sport_rx_bridge)
    {
        sport_drv_mclk_enable(mclk_enable);
        sport_drv_mclk_conifg_set(sport_mclk_pinmux, freq_mclk);
    }

    sport_drv_clock_src_sel(sport_id, SPORT_CLK_XTAL);
    sport_drv_enable_module(sport_id, enable);
    /* Select SPORT RX connected CODEC. SPORT TX will be forked to pinmux
     * when sport_tx_bridge is External, so cannot be selected individually.
     */
    sport_drv_codec_sel(sport_id, !rx_bridge);
    sport_drv_reset(sport_id);
    sport_drv_misc_config_default(&sport_misc_cfg);
    sport_misc_cfg.role = sport_role;
    sport_drv_misc_config_set(sport_id, &sport_misc_cfg);

    Pinmux_Config(sport_lrc_pinmux, pin_func_map[sport_id].lrc_pin_func);
    Pinmux_Config(sport_bclk_pinmux, pin_func_map[sport_id].bclk_pin_func);
    Pinmux_Config(sport_adc_dat_pinmux,
                  rx_bridge ? pin_func_map[sport_id].adcdat_pin_func : SDO_CODEC_SLAVE);
    Pinmux_Config(sport_dac_dat_pinmux, pin_func_map[sport_id].dacdat_pin_func);

    Pad_Config(sport_lrc_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(sport_bclk_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(sport_adc_dat_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(sport_dac_dat_pinmux, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    DIPC_PRINT_TRACE5("sport_mgr_init_param: sport_id %d, enable %d, sport_role %d, tx_bridge %d, rx_bridge %d",
                      sport_id, enable, sport_role, tx_bridge, rx_bridge);
    DIPC_PRINT_TRACE7("sport_mgr_init_param: sport_lrc_pinmux %d, sport_bclk_pinmux %d, sport_adcdat_pinmux %d, sport_dacdat_pinmux %d, "
                      "mclk_enable %d, mclk_pinmux %d, mclk_rate %f",
                      sport_lrc_pinmux, sport_bclk_pinmux, sport_adc_dat_pinmux, sport_dac_dat_pinmux,
                      mclk_enable, sport_mclk_pinmux, freq_mclk);
}

bool sport_mgr_enable(T_SPORT_MGR_SESSION_HANDLE handle)
{
    T_SPORT_MGR_SESSION *session;
    T_SPORT_MGR_CFG *sport_cfg;
    T_SPORT_PATH_DB *rx_db;
    T_SPORT_PATH_DB *tx_db;

    session = (T_SPORT_MGR_SESSION *)handle;
    sport_cfg = session->param;

    for (uint8_t i = 0; i < session->path_num; ++i)
    {
        rx_db = sport_mgr_path_db_get(sport_cfg[i].id, SPORT_PATH_AD);
        tx_db = sport_mgr_path_db_get(sport_cfg[i].id, SPORT_PATH_DA);

        if ((rx_db->ref == 0) && (tx_db->ref == 0))
        {
            sport_mgr_param_init(sport_cfg[i].id);
        }

        if (sport_cfg[i].rtx & SPORT_PATH_AD)
        {
            if (rx_db->ref == 0)
            {
                rx_db->channel_mode = (T_SPORT_CHANNEL_MODE)sport_cfg[i].rx_chann_mode;
                rx_db->sample_rate = sport_cfg[i].rx_sample_rate;

                if (tx_db->ref == 0)
                {
                    if (rx_db->bridge || tx_db->bridge)
                    {
                        tx_db->sample_rate = rx_db->sample_rate;
                    }
                }
            }
        }

        if (sport_cfg[i].rtx & SPORT_PATH_DA)
        {
            if (tx_db->ref == 0)
            {
                tx_db->channel_mode = (T_SPORT_CHANNEL_MODE)sport_cfg[i].tx_chann_mode;
                tx_db->sample_rate =  sport_cfg[i].tx_sample_rate;

                if (rx_db->ref == 0)
                {
                    if (rx_db->bridge || tx_db->bridge)
                    {
                        rx_db->sample_rate = tx_db->sample_rate;
                    }
                }
            }
        }

        sport_mgr_param_set(sport_cfg[i].id);

        if (sport_cfg[i].rtx & SPORT_PATH_AD)
        {
            rx_db->ref++;
        }

        if (sport_cfg[i].rtx & SPORT_PATH_DA)
        {
            tx_db->ref++;
        }

        DIPC_PRINT_TRACE4("sport_mgr_enable: sport_handle %d, sport_path 0x%02x, sport_tx_ref %d, sport_rx_ref %d",
                          sport_cfg[i].id, sport_cfg[i].rtx, tx_db->ref, rx_db->ref);
    }

    return true;
}

bool sport_mgr_disable(T_SPORT_MGR_SESSION_HANDLE handle)
{
    T_SPORT_MGR_CFG *sport_cfg;
    T_SPORT_MGR_SESSION *session;
    T_SPORT_PATH_DB *rx_db;
    T_SPORT_PATH_DB *tx_db;
    T_SPORT_INIT_CFG sport_init_cfg;

    session = (T_SPORT_MGR_SESSION *)handle;
    sport_cfg = session->param;

    for (uint8_t i = 0; i < session->path_num; ++i)
    {
        rx_db = sport_mgr_path_db_get(sport_cfg[i].id, SPORT_PATH_AD);
        tx_db = sport_mgr_path_db_get(sport_cfg[i].id, SPORT_PATH_DA);

        if (sport_cfg[i].rtx & SPORT_PATH_AD)
        {
            if (rx_db->ref)
            {
                rx_db->ref--;
            }
        }

        if (sport_cfg[i].rtx & SPORT_PATH_DA)
        {
            if (tx_db->ref)
            {
                tx_db->ref--;
            }
        }

        if ((rx_db->ref == 0) &&
            (tx_db->ref == 0))
        {
            sport_init_cfg = sport_mgr_init_cfg_get(sport_cfg[i].id);

            Pad_Config(sport_init_cfg.sport_lrc_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN,
                       PAD_OUT_DISABLE, PAD_OUT_HIGH);
            Pad_Config(sport_init_cfg.sport_bclk_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN,
                       PAD_OUT_DISABLE, PAD_OUT_HIGH);
            Pad_Config(sport_init_cfg.sport_adc_dat_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN,
                       PAD_OUT_DISABLE, PAD_OUT_HIGH);
            Pad_Config(sport_init_cfg.sport_dac_dat_pinmux, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN,
                       PAD_OUT_DISABLE, PAD_OUT_HIGH);

            sport_drv_enable_module((T_SPORT_ID)sport_cfg[i].id, DISABLE);
        }

        DIPC_PRINT_TRACE4("sport_mgr_disable: sport_handle %d, sport_path 0x%02x, sport_tx_ref %d, sport_rx_ref %d",
                          sport_cfg[i].id, sport_cfg[i].rtx, tx_db->ref, rx_db->ref);

    }
    return true;
}

T_SPORT_MGR_SESSION_HANDLE sport_mgr_session_create(T_AUDIO_CATEGORY category,
                                                    uint8_t          tx_chann_num,
                                                    uint8_t          rx_chann_num,
                                                    uint32_t         tx_sample_rate,
                                                    uint32_t         rx_sample_rate)
{
    T_SPORT_MGR_SESSION *session;
    T_SPORT_PATH_GROUP sport_path_group;
    T_SPORT_MGR_CHANN_MODE  rx_chann_mode;
    T_SPORT_MGR_CHANN_MODE  tx_chann_mode;
    uint8_t sport_handle;
    uint8_t sport_direction;
    int32_t ret = 0;

    session = os_mem_alloc2(sizeof(T_SPORT_MGR_SESSION));
    if (session == NULL)
    {
        ret = 1;
        goto fail_alloc_session;
    }

    sport_path_group = sport_mgr_sport_path_take(category);
    if (sport_path_group.sport_path_number == 0)
    {
        ret = 2;
        goto fail_take_sport;
    }

    session->param = os_mem_zalloc2(sport_path_group.sport_path_number * sizeof(T_SPORT_MGR_CFG));
    if (session->param == NULL)
    {
        ret = 3;
        goto fail_alloc_sport;
    }

    session->category = category;
    session->path_num = sport_path_group.sport_path_number;

    tx_chann_mode = (tx_chann_num == 1) ? SPORT_MGR_CHANN_MODE_MONO : SPORT_MGR_CHANN_MODE_STEREO;
    rx_chann_mode = (rx_chann_num == 1) ? SPORT_MGR_CHANN_MODE_MONO : SPORT_MGR_CHANN_MODE_STEREO;

    for (uint8_t i = 0; i < sport_path_group.sport_path_number; ++i)
    {
        sport_handle = sport_path_group.sport_id[i];
        sport_direction = sport_path_group.sport_direction[i];
        session->param[i].id = sport_handle;
        session->param[i].rtx = sport_direction;

        if (sport_direction & SPORT_PATH_DA)
        {
            T_AUDIO_ROUTE_SPORT_CFG tx;
            session->param[i].tx_chann_mode  = tx_chann_mode;
            session->param[i].tx_sample_rate = tx_sample_rate;
            tx = audio_route_sport_cfg_get((T_AUDIO_ROUTE_SPORT_IDX)sport_handle, AUDIO_ROUTE_SPORT_CH_DIR_TX);
            if (tx.sample_rate)
            {
                session->param[i].tx_sample_rate = tx.sample_rate;
            }
        }

        if (sport_direction & SPORT_PATH_AD)
        {
            T_AUDIO_ROUTE_SPORT_CFG rx;
            session->param[i].rx_chann_mode  = rx_chann_mode;
            session->param[i].rx_sample_rate = rx_sample_rate;
            rx = audio_route_sport_cfg_get((T_AUDIO_ROUTE_SPORT_IDX)sport_handle, AUDIO_ROUTE_SPORT_CH_DIR_RX);
            if (rx.sample_rate)
            {
                session->param[i].rx_sample_rate = rx.sample_rate;
            }
        }
    }

    sport_mgr_sport_path_give(&sport_path_group);

    os_queue_in(&(sport_mgr_db.session_list), session);

    return (T_SPORT_MGR_SESSION_HANDLE)session;

fail_alloc_sport:
fail_take_sport:
    os_mem_free(session);
fail_alloc_session:
    DIPC_PRINT_ERROR2("sport_mgr_session_create: failed %d, category %u", -ret, category);
    return NULL;
}

bool sport_mgr_destroy(T_SPORT_MGR_SESSION_HANDLE handle)
{
    T_SPORT_MGR_SESSION *session;

    session = (T_SPORT_MGR_SESSION *)handle;

    if (session->param != NULL)
    {
        os_mem_free(session->param);
    }

    os_queue_delete(&(sport_mgr_db.session_list), session);

    os_mem_free(session);

    return true;
}

uint8_t sport_mgr_tx_channel_mode_get(T_SPORT_MGR_SESSION_HANDLE handle)
{
    uint32_t sport_handle;
    uint32_t sport_path;
    T_SPORT_MGR_SESSION *session;
    T_SPORT_PATH_DB *sport_path_db;
    uint8_t channel_mode = 0;

    session = (T_SPORT_MGR_SESSION *)handle;

    for (uint8_t i = 0; i < session->path_num; ++i)
    {
        sport_handle = session->param[i].id;
        sport_path   = session->param[i].rtx;

        if (sport_path & SPORT_PATH_DA)
        {
            sport_path_db = sport_mgr_path_db_get(sport_handle, SPORT_PATH_DA);
            if (sport_path_db != NULL)
            {
                channel_mode = sport_path_db->channel_mode;
                break;
            }
        }
    }

    return channel_mode;
}

uint8_t sport_mgr_rx_channel_mode_get(T_SPORT_MGR_SESSION_HANDLE handle)
{
    uint32_t sport_handle;
    uint32_t sport_path;
    T_SPORT_MGR_SESSION *session;
    T_SPORT_PATH_DB *sport_path_db;
    uint8_t channel_mode = 0;

    session = (T_SPORT_MGR_SESSION *)handle;

    for (uint8_t i = 0; i < session->path_num; ++i)
    {
        sport_handle = session->param[i].id;
        sport_path   = session->param[i].rtx;

        if (sport_path & SPORT_PATH_AD)
        {
            sport_path_db = sport_mgr_path_db_get(sport_handle, SPORT_PATH_AD);
            if (sport_path_db != NULL)
            {
                channel_mode = sport_path_db->channel_mode;
                break;
            }
        }
    }

    return channel_mode;
}
