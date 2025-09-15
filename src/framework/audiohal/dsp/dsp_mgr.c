/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "os_mem.h"
#include "trace.h"
#include "wdg.h"
#include "pm.h"
#include "sys_event.h"
#include "audio_route.h"
#include "audio_codec.h"
#include "sport_mgr.h"
#include "dsp_driver.h"
#include "dsp_mgr.h"
#include "dsp_loader.h"

/* TODO Remove Start */
#include "sport_driver.h"
#include "dsp_shm.h"
#include "fmc_api.h"
#include "sys_cfg.h"
#include "remote.h"
#include "bt_mgr.h"
#include "audio_type.h"
#include "section.h"
#if (TARGET_RTL8773D == 1)
#include "shm2_api.h"
#include "ext_buck.h"
#endif

#define VAD_BASE_ADDR       (0x40021600)

#define DSP_MGR_EVENT_FLAG_FADE_OUT                    (0x00000001)
#define DSP_MGR_EVENT_FLAG_VOICE_ALGO_PARAM            (0x00000002)
#define DSP_MGR_EVENT_FLAG_AUDIO_ALGO_PARAM            (0x00000004)
#define DSP_MGR_EVENT_FLAG_WAIT_LOAD                   (0x00000008)
#define DSP_MGR_EVENT_FLAG_WAIT_NOTIFICATION_FINISH    (0x00000010)
#define DSP_MGR_EVENT_FLAG_NOTIFICATION_START          (0x00000020)


#define DSP_MGR_EVENT_FLAG_GATE0_SYNC_PROC             (0x00000100)
#define DSP_MGR_EVENT_FLAG_GATE1_SYNC_PROC             (0x00000200)
#define DSP_MGR_EVENT_FLAG_GATE2_SYNC_PROC             (0x00000400)

#define DSP_MGR_EVENT_FLAG_WAIT_GATE0_RX_READY         (0x00010000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE0_TX_READY         (0x00020000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE0_RX_STOP          (0x00040000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE0_TX_STOP          (0x00080000)

#define DSP_MGR_EVENT_FLAG_WAIT_GATE1_RX_READY         (0x00100000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE1_TX_READY         (0x00200000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE1_RX_STOP          (0x00300000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE1_TX_STOP          (0x00800000)

#define DSP_MGR_EVENT_FLAG_WAIT_GATE2_RX_READY         (0x01000000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE2_TX_READY         (0x02000000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE2_RX_STOP          (0x04000000)
#define DSP_MGR_EVENT_FLAG_WAIT_GATE2_TX_STOP          (0x08000000)

#define DSP_MSG_SUB_TYPE_NOTIFICATION_END                         (0)

#define SPORT_DIRECTION_NONE             (0x00)
#define SPORT_DIRECTION_DA               (0x01)
#define SPORT_DIRECTION_AD               (0x02)

#define DSP_APP_A2DP    (0)
#define DSP_APP_VOICE   (1)

typedef enum
{
    DSP_SUBSTATE_IDLE               = 0x00,
    DSP_SUBSTATE_STARTING           = 0x01,
    DSP_SUBSTATE_RUNNING            = 0x02,
    DSP_SUBSTATE_STOPPING           = 0x03,
} T_DSP_SUBSTATE;

typedef enum t_dsp_session_state
{
    DSP_SESSION_STATE_IDLE        = 0x00,
    DSP_SESSION_STATE_STARTING    = 0x01,
    DSP_SESSION_STATE_RUNNING     = 0x02,
    DSP_SESSION_STATE_STOPPING    = 0x03,
} T_DSP_SESSION_STATE;

typedef struct
{
    uint32_t bypass_near_end : 1;
    uint32_t bypass_far_end : 1;
    uint32_t aec : 1;
    uint32_t dmnr : 1;
    uint32_t aes : 1;
    uint32_t nr_near_end : 1;
    uint32_t nr_far_end : 1;
    uint32_t pitch_det_near_end : 1;
    uint32_t pitch_det_far_end : 1;
    uint32_t drc_near_end : 1;
    uint32_t drc_far_end : 1;
    uint32_t rmdc_near_end : 1;
    uint32_t hpf_near_end : 1;
    uint32_t hpf_far_end : 1;
    uint32_t eq_near_end8k : 1;
    uint32_t eq_near_end16k : 1;
    uint32_t eq_far_end8k : 1;
    uint32_t eq_far_end16k : 1;
    uint32_t ignore_call_active_flag : 1;
    uint32_t dmnr_post_process : 1;
    uint32_t dmnr_test_mode : 1;
    uint32_t dmnr_bypass_lorr : 1;//0 for L,1 for R,effect if Dmnr and DmnrTestMode are 1
    uint32_t side_tone : 1;
    uint32_t far_end_loopback : 1;
    uint32_t dmnr_middle : 1; // 0: follow the setting of DmnrByPassLorR, 1: output L/2+R/2
    uint32_t nr_near_end128pts : 1;
    uint32_t bone_fusion : 1;
    uint32_t resv : 5;
} T_VOICE_PROC_ENB;

typedef struct
{
    // audio effect
    uint32_t bypass_audio_effect : 1;
    uint32_t mbagc_audio : 1;
    uint32_t eq_audio : 1;
    uint32_t aw_audio : 1;

    // audio pass through
    uint32_t audio_pass_through : 1;
    uint32_t vad_trigger : 1;
    uint32_t dehowling : 1;
    uint32_t nr_near_end : 1;

    uint32_t drc_near_end : 1;
    uint32_t rmdc_near_end : 1;
    uint32_t hpf_near_end : 1;
    uint32_t eq_near_end : 1;

    uint32_t fw_dsp_mix : 1;
    uint32_t bypass_near_end_effect : 1;
    uint32_t limiter : 1;
    uint32_t apt_mic_select : 1; // 0 for mic0, 1 for mic1

    uint32_t aec_alr : 1;
    uint32_t aes512 : 1;
    uint32_t resv : 14;
} T_AUDIO_PROC_ENB;

typedef struct t_dsp_param_r_only
{
    uint16_t sync_word;
    uint16_t reserved1;
    uint32_t tool_version_info;
    uint16_t user_version;
    uint16_t reserved2;
    uint32_t algo_block_offset;
    uint32_t eq_cmd_block_offset;
    uint32_t gain_table_block_offset;
    uint32_t vad_param_block_offset;
    uint32_t eq_extend_info_offset;
    uint32_t hw_eq_block_offset;
    uint8_t  reserved[8];
    uint32_t package_features; //need check it in patch
    T_VOICE_PROC_ENB voice_stream_feature_bit;
    T_AUDIO_PROC_ENB audio_stream_feature_bit;
} T_DSP_PARAM_R_ONLY;

typedef struct t_dsp_algo_param_header
{
    uint32_t offset;
    uint32_t sample_rate;
    uint8_t  scenario;
    uint8_t  eq_type;
    uint16_t cmd_length;
} T_DSP_ALGO_PARAM_HEADER;

typedef struct t_dsp_algo_block
{
    uint16_t sync_word;
    uint16_t reserved1;
    uint32_t algo_block_len;
    uint16_t algo_num;
    uint16_t reserved2;
    T_DSP_ALGO_PARAM_HEADER *param_header;
} T_DSP_ALGO_BLOCK;

typedef struct t_dsp_vad_param_header
{
    uint16_t    sync_word;
    uint16_t    reserved1;
    uint8_t     reserved2;
    uint8_t     scenario_num;
    uint16_t    each_scenario_len;
    uint32_t    vad_len;
} T_DSP_VAD_PARAM_HEADER;

typedef struct t_dsp_vad_param
{
    T_DSP_VAD_PARAM_HEADER *header;
    uint8_t *param;
} T_DSP_VAD_PARAM;

typedef enum
{
    DSP_EVENT_POWER_DOWN_DSP                = 0x01,
    DSP_EVENT_POWER_ON_DSP                  = 0x02,
    DSP_EVENT_D2H_POWER_DOWN_ACK            = 0x15,
    DSP_EVENT_IMAGE_LOAD_FINISH             = 0x23,

    DSP_EVENT_START_FW                      = 0x52,
    DSP_EVENT_STOP_FW                       = 0x53,

} T_DSP_EVENT;

typedef struct t_dsp_mgr_session
{
    struct t_dsp_mgr_session       *p_next;
    void                           *context;
    P_DSP_MGR_SESSION_CBACK         callback;
    T_SPORT_MGR_SESSION_HANDLE      sport_handle;
    T_DSP_IPC_DECODER              *decoder_param;
    T_DSP_IPC_ENCODER              *encoder_param;
    T_DSP_SBC_ENCODE_PARAM         *sbc_param;
    uint32_t                        src_transport_address;
    uint32_t                        snk_transport_address;
    T_DSP_DATA_MODE                 data_mode;
    uint8_t                         type;
    T_DSP_SESSION_STATE             state;
} T_DSP_MGR_SESSION;

typedef struct t_decode_context
{
    bool busy;
    T_DSP_MGR_SESSION *owner;
} T_DECODE_CONTEXT;

typedef struct t_encode_context
{
    bool busy;
    T_DSP_MGR_SESSION *owner;
} T_ENCODE_CONTEXT;

typedef struct t_gate_rtx_db
{
    T_DSP_SUBSTATE  state;
    uint8_t         ref_cnt;
} T_GATE_RTX_DB;

typedef struct t_gate_state
{
    T_GATE_RTX_DB gate_rtx[DIPC_DIRECTION_MAX];
} T_GATE_STATE;

typedef struct t_dsp_mgr_db
{
    T_SYS_IPC_HANDLE            event;
    T_SYS_EVENT_GROUP_HANDLE    event_group;
    T_OS_QUEUE                  session_queue;
    T_DECODE_CONTEXT            decoder;
    T_ENCODE_CONTEXT            encoder;
    T_DSP_STATE                 dsp_state;
    T_GATE_STATE                gate_state[DIPC_GATE_MAX];
    T_DSP_VAD_PARAM             vad_param;
    uint32_t                    vad_param_block_offset;
    uint32_t                    algo_block_offset;
    T_DSP_ALGO_BLOCK            algo_block;
    uint8_t                     sport0_da_chann;
    uint8_t                     sport0_ad_chann;
    uint8_t                     dsp2_ref;
} T_DSP_MGR_DB;

typedef bool (*P_IO_FLITER_CBACK)(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path);
typedef bool (*P_ACTION_CBACK)(T_ACTION_CONFIG action, T_DSP_IPC_LOGIC_IO *logic_io);
typedef uint32_t (*P_DEPLOYMENT_CBACK)(uint8_t gate_id, uint8_t direction,
                                       T_DSP_MGR_SESSION *session);
typedef void (*P_DEPLOYMENT_POST_CBACK)(T_DSP_MGR_SESSION *session, uint32_t wait_flag);
typedef void (*P_DSP_MGR_SESSION_CFG_CBACK)(T_DSP_MGR_SESSION_HANDLE handle);
typedef bool (*P_NOTIFICATION_CBACK)(void);

static void dsp_download_algo_param(T_DSP_MGR_SESSION *session);
bool dsp_lps_check(void);
void dsp_state_machine(uint8_t event, uint32_t parameter);
static void dsp_analog_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_notification_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_audio_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_voice_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_apt_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_vad_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_anc_llapt_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static void dsp_record_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
static bool dsp_ipc_event_handler(T_DSP_IPC_EVENT event, uint32_t param);
static void dsp_pipe_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle);
/* TODO Remove End */

const static uint32_t gate_wait_rx_ready_flag[] =
{
    [DIPC_GATE_ID0] = DSP_MGR_EVENT_FLAG_WAIT_GATE0_RX_READY,
    [DIPC_GATE_ID1] = DSP_MGR_EVENT_FLAG_WAIT_GATE1_RX_READY,
    [DIPC_GATE_ID2] = DSP_MGR_EVENT_FLAG_WAIT_GATE2_RX_READY,
};

const static uint32_t gate_wait_tx_ready_flag[] =
{
    [DIPC_GATE_ID0] = DSP_MGR_EVENT_FLAG_WAIT_GATE0_TX_READY,
    [DIPC_GATE_ID1] = DSP_MGR_EVENT_FLAG_WAIT_GATE1_TX_READY,
    [DIPC_GATE_ID2] = DSP_MGR_EVENT_FLAG_WAIT_GATE2_TX_READY,
};

const static uint32_t gate_wait_rx_stop_flag[] =
{
    [DIPC_GATE_ID0] = DSP_MGR_EVENT_FLAG_WAIT_GATE0_RX_STOP,
    [DIPC_GATE_ID1] = DSP_MGR_EVENT_FLAG_WAIT_GATE1_RX_STOP,
    [DIPC_GATE_ID2] = DSP_MGR_EVENT_FLAG_WAIT_GATE2_RX_STOP,
};

const static uint32_t gate_wait_tx_stop_flag[] =
{
    [DIPC_GATE_ID0] = DSP_MGR_EVENT_FLAG_WAIT_GATE0_TX_STOP,
    [DIPC_GATE_ID1] = DSP_MGR_EVENT_FLAG_WAIT_GATE1_TX_STOP,
    [DIPC_GATE_ID2] = DSP_MGR_EVENT_FLAG_WAIT_GATE2_TX_STOP,
};

static const uint32_t gate_sig_proc_flag[] =
{
    [DIPC_GATE_ID0] = DSP_MGR_EVENT_FLAG_GATE0_SYNC_PROC,
    [DIPC_GATE_ID1] = DSP_MGR_EVENT_FLAG_GATE1_SYNC_PROC,
    [DIPC_GATE_ID2] = DSP_MGR_EVENT_FLAG_GATE2_SYNC_PROC,
};

static const uint8_t sport_data_len_table[] =
{
    [AUDIO_ROUTE_SPORT_DATA_LEN_8_BIT]  = 0x08,
    [AUDIO_ROUTE_SPORT_DATA_LEN_16_BIT] = 0x10,
    [AUDIO_ROUTE_SPORT_DATA_LEN_20_BIT] = 0x14,
    [AUDIO_ROUTE_SPORT_DATA_LEN_24_BIT] = 0x18,
    [AUDIO_ROUTE_SPORT_DATA_LEN_32_BIT] = 0x20,
};

const P_DSP_MGR_SESSION_CFG_CBACK cfg_cback[DSP_SESSION_TYPE_NUMBER] =
{
    /* AUDIO */
    [DSP_SESSION_TYPE_AUDIO]  = dsp_audio_cfg_setting,
    /* VOICE */
    [DSP_SESSION_TYPE_VOICE]  = dsp_voice_cfg_setting,
    /* RECORD */
    [DSP_SESSION_TYPE_RECORD] = dsp_record_cfg_setting,
    /* ANALOG */
    [DSP_SESSION_TYPE_ANALOG] = dsp_analog_cfg_setting,
    /* TONE */
    [DSP_SESSION_TYPE_TONE]   = dsp_notification_cfg_setting,
    /* VP */
    [DSP_SESSION_TYPE_VP]     = dsp_notification_cfg_setting,
    /* APT */
    [DSP_SESSION_TYPE_APT]    = dsp_apt_cfg_setting,
    /* LLAPT */
    [DSP_SESSION_TYPE_LLAPT]  = dsp_anc_llapt_cfg_setting,      /* unused */
    /* ANC */
    [DSP_SESSION_TYPE_ANC]    = dsp_anc_llapt_cfg_setting,      /* unused */
    /* VAD */
    [DSP_SESSION_TYPE_VAD]    = dsp_vad_cfg_setting,      /* unused */

    [DSP_SESSION_TYPE_PIPE] = dsp_pipe_cfg_setting,
};

static T_DSP_MGR_DB dsp_mgr_db;

static T_SYS_EVENT_GROUP_HANDLE dsp_mgr_event_group_get(void)
{
    return dsp_mgr_db.event_group;
}

static T_DSP_STATE dsp_mgr_dsp_state_get(void)
{
    return dsp_mgr_db.dsp_state;
}

static void dsp_mgr_dsp_state_set(T_DSP_STATE state)
{
    dsp_mgr_db.dsp_state = state;

    return ;
}

static T_DSP_SUBSTATE dsp_mgr_sub_state_get(uint8_t id, uint8_t direction)
{
    T_DSP_SUBSTATE state;

    state = DSP_SUBSTATE_IDLE;

    state = dsp_mgr_db.gate_state[id].gate_rtx[direction].state;

    return state;
}

static bool dsp_mgr_sub_state_set(uint8_t id, uint8_t direction, T_DSP_SUBSTATE state)
{

    dsp_mgr_db.gate_state[id].gate_rtx[direction].state = state;

    return true;
}

void dsp_vad_param_load(void)
{
#if 0
    T_DSP_VAD_PARAM_HEADER *header;
    uint32_t data_offset;
    uint32_t vad_param_length;

    dsp_mgr_db.vad_param.header = os_mem_zalloc2(sizeof(T_DSP_VAD_PARAM_HEADER));
    header = dsp_mgr_db.vad_param.header;
    if (header == NULL)
    {
        return;
    }
    header->sync_word = 0;
    data_offset = DSP_PARAM_OFFSET + dsp_mgr_db.vad_param_block_offset;
    fmc_flash_nor_read(flash_dsp_cfg_addr_get() + data_offset,
                       header, sizeof(T_DSP_VAD_PARAM_HEADER));

    DIPC_PRINT_TRACE2("dsp_vad_param_load: num %d, each length %d", header->scenario_num,
                      header->each_scenario_len);

    if (header->sync_word != APP_DATA_SYNC_WORD)
    {
        DIPC_PRINT_ERROR0("dsp_vad_param_load: Load DSP VAD parameter header fail");
        os_mem_free(header);
        return;
    }

    vad_param_length = header->scenario_num * header->each_scenario_len;
    dsp_mgr_db.vad_param.param = os_mem_zalloc2(vad_param_length);
    if (dsp_mgr_db.vad_param.param == NULL)
    {
        os_mem_free(header);
        return;
    }

    data_offset += sizeof(T_DSP_VAD_PARAM_HEADER);
    fmc_flash_nor_read(flash_dsp_cfg_addr_get() + data_offset,
                       dsp_mgr_db.vad_param.param, vad_param_length);
#endif
}

void dsp_algo_param_load(void)
{
    T_DSP_ALGO_BLOCK *block;
    uint32_t data_offset;
    int32_t ret = 0;

    block = &dsp_mgr_db.algo_block;
    data_offset = DSP_PARAM_OFFSET + dsp_mgr_db.algo_block_offset;

    fmc_flash_nor_read(flash_dsp_cfg_addr_get() + data_offset,
                       block, sizeof(T_DSP_ALGO_BLOCK) - sizeof(T_DSP_ALGO_PARAM_HEADER *));
    if (block->sync_word != APP_DATA_SYNC_WORD)
    {
        ret = 1;
        goto fail_load_block;
    }

#if F_APP_EXT_MIC_SWITCH_SUPPORT
    static T_DSP_ALGO_PARAM_HEADER *prev_param_header = NULL;

    if (prev_param_header == NULL)
    {
        block->param_header = calloc(block->algo_num, sizeof(T_DSP_ALGO_PARAM_HEADER));

        if (block->param_header != NULL)
        {
            prev_param_header = block->param_header;
        }
    }
    else
    {
        block->param_header = prev_param_header;
    }
#else
    block->param_header = calloc(block->algo_num, sizeof(T_DSP_ALGO_PARAM_HEADER));
#endif

    if (block->param_header == NULL)
    {
        ret = 2;
        goto fail_alloc_header;
    }

    data_offset += sizeof(T_DSP_ALGO_BLOCK) - sizeof(T_DSP_ALGO_PARAM_HEADER *);
    if (fmc_flash_nor_read(flash_dsp_cfg_addr_get() + data_offset,
                           block->param_header, block->algo_num * sizeof(T_DSP_ALGO_PARAM_HEADER)) == false)
    {
        ret = 3;
        goto fail_load_header;
    }

    DIPC_PRINT_TRACE2("dsp_algo_param_load: algo_block_len %d, algo_num %d",
                      block->algo_block_len, block->algo_num);

    return;

fail_load_header:
    os_mem_free(block->param_header);
    block->param_header = NULL;
fail_alloc_header:
fail_load_block:
    DIPC_PRINT_ERROR1("dsp_algo_param_load: failed %d", -ret);
}

bool dsp_param_init(void)
{
    T_DSP_PARAM_R_ONLY *dsp_param;
    int32_t ret = 0;

    dsp_param = os_mem_alloc2(sizeof(T_DSP_PARAM_R_ONLY));
    if (dsp_param == NULL)
    {
        ret = 1;
        goto fail_alloc;
    }

    fmc_flash_nor_read(flash_dsp_cfg_addr_get() + DSP_PARAM_OFFSET,
                       dsp_param, sizeof(T_DSP_PARAM_R_ONLY));

    if (dsp_param->sync_word != APP_DATA_SYNC_WORD)
    {
        ret = 2;
        goto fail_load;
    }

    dsp_mgr_db.algo_block_offset = dsp_param->algo_block_offset;
    dsp_mgr_db.vad_param_block_offset = dsp_param->vad_param_block_offset;
    dsp_algo_param_load();
    os_mem_free(dsp_param);

    return true;

fail_load:
    os_mem_free(dsp_param);
fail_alloc:
    DIPC_PRINT_ERROR1("dsp_param_init: failed %d", -ret);
    return false;
}

static void dsp_mgr_wait_fw_ready_handler(void *handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    if (session)
    {
        session->state = DSP_SESSION_STATE_RUNNING;

        if (session->callback != NULL)
        {
            session->callback(session->context, DSP_MGR_EVT_FW_READY, 0);
        }
    }
}

static void dsp_mgr_deployment_traverse(T_DSP_MGR_SESSION *session,
                                        P_DEPLOYMENT_CBACK cback,
                                        P_DEPLOYMENT_POST_CBACK post_cback)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group;
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    uint8_t gate_map[DIPC_GATE_MAX] = {0};
    uint32_t wait_flag;
    uint8_t i;

    if (session->type >= DSP_SESSION_TYPE_PIPE)
    {
        return ;
    }

    physical_path_group = audio_route_physical_path_take((T_AUDIO_CATEGORY)session->type);

    if (physical_path_group.physical_path_num == 0)
    {
        goto func_end;
    }

    physical_path = physical_path_group.physical_path;

    for (i = 0; i < physical_path_group.physical_path_num; ++i)
    {
        if ((session->type == DSP_SESSION_TYPE_APT) &&
            ((physical_path[i].logic_io_type == AUDIO_ROUTE_LOGIC_PRIMARY_REF_SPEAKER) || \
             (physical_path[i].logic_io_type == AUDIO_ROUTE_LOGIC_SECONDARY_REF_SPEAKER)))
        {
            continue;
        }
        else if (physical_path[i].sport_idx < DIPC_GATE_MAX)
        {
            gate_map[physical_path[i].sport_idx] |=
                ((physical_path[i].sport_ch_dir == AUDIO_ROUTE_SPORT_CH_DIR_TX) ?
                 BIT(DIPC_DIRECTION_TX) : BIT(DIPC_DIRECTION_RX));
        }
    }

    wait_flag = 0;

    for (i = 0; i < DIPC_GATE_MAX; ++i)
    {
        if (gate_map[i] != 0)
        {
            wait_flag |= cback(i, gate_map[i], session);
        }
    }

    post_cback(session, wait_flag);

func_end:

    audio_route_physical_path_give(&physical_path_group);

    return ;
}

void dsp_mgr_sport_start_post_cback(T_DSP_MGR_SESSION *session, uint32_t wait_flag)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;

    event_group = dsp_mgr_event_group_get();

    if (wait_flag == 0)
    {
        dsp_send_msg(DSP_MSG_SPORT_START, 0, session, 0);
    }
    else
    {
        sys_event_flag_post(event_group,
                            SYS_EVENT_FLAG_OPT_SET,
                            wait_flag);

        sys_event_flag_wait(event_group,
                            dsp_mgr_wait_fw_ready_handler,
                            session,
                            wait_flag,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
    }
}

uint32_t dsp_mgr_sport_start(uint8_t gate_id, uint8_t direction, T_DSP_MGR_SESSION *session)
{
    T_DSP_SUBSTATE ad_state;
    T_DSP_SUBSTATE da_state;
    uint32_t wait_flag;

    ad_state = dsp_mgr_sub_state_get(gate_id, DIPC_DIRECTION_RX);
    da_state = dsp_mgr_sub_state_get(gate_id, DIPC_DIRECTION_TX);
    wait_flag = 0;

    if ((ad_state == DSP_SUBSTATE_IDLE) &&
        (da_state == DSP_SUBSTATE_IDLE))
    {
        dsp_download_algo_param(session);
    }

    if (direction == (BIT(DIPC_DIRECTION_TX) | BIT(DIPC_DIRECTION_RX)))
    {
        (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt)++;
        (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt)++;

        if ((ad_state == DSP_SUBSTATE_IDLE) || (da_state == DSP_SUBSTATE_IDLE))
        {
            T_DSP_MGR_EVENT_MSG msg;
            msg.algo = session->decoder_param->algorithm;
            msg.category = (T_AUDIO_CATEGORY)session->type;

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_EFFECT_REQ, &msg);

            if ((ad_state == DSP_SUBSTATE_IDLE) && (da_state == DSP_SUBSTATE_IDLE))
            {
                dipc_gate_start(gate_id, direction);
            }
            else if (ad_state == DSP_SUBSTATE_IDLE)
            {
                dipc_gate_start(gate_id, (direction & BIT(DIPC_DIRECTION_RX)));
            }
            else if (da_state == DSP_SUBSTATE_IDLE)
            {
                dipc_gate_start(gate_id, (direction & BIT(DIPC_DIRECTION_TX)));
            }

            if (ad_state == DSP_SUBSTATE_IDLE)
            {
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_STARTING);
                wait_flag |= gate_wait_rx_ready_flag[gate_id];
            }

            if (da_state == DSP_SUBSTATE_IDLE)
            {
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_STARTING);
                wait_flag |= gate_wait_tx_ready_flag[gate_id];
            }
        }
    }
    else if (direction == BIT(DIPC_DIRECTION_TX))
    {
        (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt)++;

        if (da_state == DSP_SUBSTATE_IDLE)
        {
            T_DSP_MGR_EVENT_MSG msg;
            msg.algo = session->decoder_param->algorithm;
            msg.category = (T_AUDIO_CATEGORY)session->type;

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_EFFECT_REQ, &msg);

            dipc_gate_start(gate_id, direction);
            dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_STARTING);
            wait_flag |= gate_wait_tx_ready_flag[gate_id];
        }
    }
    else if (direction == BIT(DIPC_DIRECTION_RX))
    {
        (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt)++;

        if (ad_state == DSP_SUBSTATE_IDLE)
        {
            T_DSP_MGR_EVENT_MSG msg;
            msg.algo = session->encoder_param->algorithm;
            msg.category = (T_AUDIO_CATEGORY)session->type;

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_EFFECT_REQ, &msg);

            dipc_gate_start(gate_id, direction);
            dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_STARTING);
            wait_flag |= gate_wait_rx_ready_flag[gate_id];
        }
    }

    DIPC_PRINT_TRACE6("dsp_mgr_sport_start: session %p, gate_id %d, "
                      "direction 0x%02x, ad_state %d, da_state %d, wait_flag 0x%08x",
                      session, gate_id, direction, ad_state, da_state, wait_flag);

    return wait_flag;
}

static bool dsp_mgr_is_idle(void)
{
    bool result = true;
    T_DSP_MGR_SESSION *session;

    session = os_queue_peek(&dsp_mgr_db.session_queue, 0);

    while (session != NULL)
    {
        if (session->state != DSP_SESSION_STATE_IDLE)
        {
            result = false;
            break;
        }
        session = session->p_next;
    }

    return result;
}

static void dsp_mgr_wait_fw_stop_handler(void *handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    if (session)
    {
        if (session->type == DSP_SESSION_TYPE_RECORD)
        {
            dsp_mgr_encoder_action(AUDIO_CATEGORY_RECORD, DSP_IPC_ACTION_STOP);
        }

        session->state = DSP_SESSION_STATE_IDLE;

        if (session->callback != NULL)
        {
            session->callback(session->context, DSP_MGR_EVT_FW_STOP, 0);
        }

        if (dsp_mgr_is_idle())
        {
            dsp_state_machine(DSP_EVENT_STOP_FW, 0);
        }
    }
}

void dsp_mgr_gate_stop_post_cback(T_DSP_MGR_SESSION *session, uint32_t wait_flag)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;

    event_group = dsp_mgr_event_group_get();

    if (wait_flag == 0)
    {
        if (session->type == DSP_SESSION_TYPE_RECORD)
        {
            dsp_mgr_encoder_action(AUDIO_CATEGORY_RECORD, DSP_IPC_ACTION_STOP);
        }

        dsp_send_msg(DSP_MSG_SPORT_STOP, 0, session, 0);
    }
    else
    {
        sys_event_flag_post(event_group,
                            SYS_EVENT_FLAG_OPT_SET,
                            wait_flag);

        sys_event_flag_wait(event_group,
                            dsp_mgr_wait_fw_stop_handler,
                            session,
                            wait_flag,
                            SYS_EVENT_FLAG_TYPE_CLEAR_AND);
    }
}

uint32_t dsp_mgr_sport_stop(uint8_t gate_id, uint8_t direction, T_DSP_MGR_SESSION *session)
{
    T_DSP_SUBSTATE ad_state;
    T_DSP_SUBSTATE da_state;
    uint32_t wait_flag;

    ad_state = dsp_mgr_sub_state_get(gate_id, DIPC_DIRECTION_RX);
    da_state = dsp_mgr_sub_state_get(gate_id, DIPC_DIRECTION_TX);
    wait_flag = 0;

    if (direction == (BIT(DIPC_DIRECTION_TX) | BIT(DIPC_DIRECTION_RX)))
    {
        if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt)
        {
            (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt)--;
        }

        if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt)
        {
            (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt)--;
        }

        if ((dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt == 0) &&
            (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt == 0))
        {
            if ((ad_state == DSP_SUBSTATE_RUNNING) &&
                (da_state == DSP_SUBSTATE_RUNNING))
            {
                dipc_gate_stop(gate_id, direction);
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_STOPPING);
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_STOPPING);
                wait_flag |= gate_wait_rx_stop_flag[gate_id];
                wait_flag |= gate_wait_tx_stop_flag[gate_id];
            }
        }
        else if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt == 0)
        {
            if (ad_state == DSP_SUBSTATE_RUNNING)
            {
                dipc_gate_stop(gate_id, (direction & BIT(DIPC_DIRECTION_RX)));
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_STOPPING);
                wait_flag |= gate_wait_rx_stop_flag[gate_id];
            }
        }
        else if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt == 0)
        {
            if (da_state == DSP_SUBSTATE_RUNNING)
            {
                dipc_gate_stop(gate_id, (direction & BIT(DIPC_DIRECTION_TX)));
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_STOPPING);
                wait_flag |= gate_wait_tx_stop_flag[gate_id];
            }
        }
    }
    else if (direction == BIT(DIPC_DIRECTION_TX))
    {
        if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt)
        {
            (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt)--;
        }

        if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt == 0)
        {
            if (da_state == DSP_SUBSTATE_RUNNING)
            {
                dipc_gate_stop(gate_id, direction);
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_STOPPING);
                wait_flag |= gate_wait_tx_stop_flag[gate_id];
            }
        }
    }
    else if (direction == BIT(DIPC_DIRECTION_RX))
    {
        if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt)
        {
            (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt)--;
        }

        if (dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt == 0)
        {
            if (ad_state == DSP_SUBSTATE_RUNNING)
            {
                dipc_gate_stop(gate_id, direction);
                dsp_mgr_sub_state_set(gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_STOPPING);
                wait_flag |= gate_wait_rx_stop_flag[gate_id];
            }
        }
    }

    DIPC_PRINT_TRACE8("dsp_mgr_sport_stop: session %p, gate_id %d, direction 0x%02x, "
                      "ad_state %d, da_state %d, wait_flag %d, ad_ref_cnt %d, da_ref_cnt %d",
                      session, gate_id, direction, ad_state, da_state, wait_flag,
                      dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_RX].ref_cnt,
                      dsp_mgr_db.gate_state[gate_id].gate_rtx[DIPC_DIRECTION_TX].ref_cnt);

    return wait_flag;
}

static void dsp_mgr_wait_fade_out_finish_cback(void *handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;
    if (session)
    {
        if (session->callback != NULL)
        {
            session->callback(session->context, DSP_MGR_EVT_FADE_OUT_FINISH, 0);
        }
    }
}

void dsp_mgr_fade_out_start(bool stream_continue, T_DSP_MGR_SESSION_HANDLE dsp_session)
{
    T_DSP_SUBSTATE sport0_da_state;
    T_SYS_EVENT_GROUP_HANDLE event_group;
    bool fading;

    sport0_da_state = dsp_mgr_sub_state_get(DIPC_GATE_ID0, DIPC_DIRECTION_TX);
    event_group = dsp_mgr_event_group_get();
    fading = sys_event_flag_check(event_group, DSP_MGR_EVENT_FLAG_FADE_OUT);

    DIPC_PRINT_TRACE3("dsp_mgr_fade_out_start: stream_state = %d, sport0_da_state %d, fading %d",
                      stream_continue, sport0_da_state, fading);

    if ((sport0_da_state == DSP_SUBSTATE_RUNNING) &&
        (stream_continue == true) &&
        (fading == false))
    {
        dsp_ipc_set_fade_in_out_control(IPC_FADE_OUT);
    }
    else
    {
        /* Voice Audio Analog do not work at same time */
        dsp_send_msg(DSP_MSG_FADE_OUT_FINISH, 0, dsp_session, 0);
    }

    sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_SET,
                        DSP_MGR_EVENT_FLAG_FADE_OUT);
    //wait node
    sys_event_flag_wait(event_group,
                        dsp_mgr_wait_fade_out_finish_cback,
                        (void *)dsp_session,
                        DSP_MGR_EVENT_FLAG_FADE_OUT,
                        SYS_EVENT_FLAG_TYPE_CLEAR_AND);
}

void dsp_mgr_start(void)
{
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
    shm2_init();
    ext_buck_vcore2_enable();
#endif
    dsp_hal_boot();

    dsp_load_initial();

    dsp_mgr_dsp_state_set(DSP_STATE_IDLE);
}

static void dsp_analog_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    dsp_ipc_set_decoder(*(session->decoder_param), false);
    dsp_ipc_set_encoder(*(session->encoder_param), false);

    dsp_ipc_set_stream_channel_out_config(dsp_mgr_db.sport0_da_chann, false);
}

static void dsp_notification_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;
    T_DSP_SUBSTATE sport0_da_state;

    session = (T_DSP_MGR_SESSION *)handle;
    sport0_da_state = dsp_mgr_sub_state_get(DIPC_GATE_ID0, DIPC_DIRECTION_TX);

    if (sport0_da_state != DSP_SUBSTATE_RUNNING)
    {
        dsp_ipc_set_decoder(*(session->decoder_param), false);
        dsp_ipc_set_stream_channel_out_config(dsp_mgr_db.sport0_da_chann, false);
    }
}

static void dsp_audio_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    if (session->data_mode == DSP_DATA_ULTRA_LOW_LATENCY_MODE)
    {
        dsp_ipc_set_fifo_len(DSP_FIFO_TX_DIRECTION, true);
        dsp_ipc_set_decode_emtpy(false);
    }

    dsp_ipc_set_data_mode(session->data_mode, session->type);
    if (session->decoder_param->algorithm == ALGORITHM_LC3)
    {
        dsp_ipc_set_lc3_decoder(session->decoder_param->comm_header, session->decoder_param->decoder);
    }
    else
    {
        /*H2D_DECODER_SET*/
        dsp_ipc_set_decoder(*(session->decoder_param), false);
    }

    dsp_ipc_set_stream_channel_out_config(dsp_mgr_db.sport0_da_chann, false);
}

static void dsp_vad_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    return ;
}

static void dsp_pipe_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    return ;
}

static void dsp_anc_llapt_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    return ;
}

static void dsp_voice_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    dsp_ipc_set_data_mode(session->data_mode, session->type);
    if (session->decoder_param->algorithm == ALGORITHM_LC3_VOICE)
    {
        dsp_ipc_set_lc3_decoder(session->decoder_param->comm_header, session->decoder_param->decoder);
    }
    else
    {
        dsp_ipc_set_decoder(*(session->decoder_param), false);
    }

    if (session->encoder_param->algorithm == ALGORITHM_LC3_VOICE)
    {
        dsp_ipc_set_lc3_encoder(session->encoder_param->comm_header, session->encoder_param->encoder);
    }
    else
    {
        dsp_ipc_set_encoder(*(session->encoder_param), false);
    }

    dsp_ipc_set_stream_channel_out_config(dsp_mgr_db.sport0_da_chann, false);
}

bool dsp_sport0_da_chann_set(uint8_t chann_mask)
{
    dsp_mgr_db.sport0_da_chann = chann_mask;

    if (dsp_mgr_db.dsp_state != DSP_STATE_FW_READY)
    {
        return false;
    }

    return dsp_ipc_set_stream_channel_out_config(chann_mask, true);
}

bool dsp_sport0_ad_chann_set(uint8_t chann_mask)
{
    dsp_mgr_db.sport0_ad_chann = chann_mask;
    return dsp_ipc_set_stream_channel_in_config(chann_mask);
}

void dsp_mgr_suppress_tx_gain(int16_t gain_step_left, uint8_t time_index)
{
    dsp_ipc_set_tx_path_ramp_gain_control((uint16_t)gain_step_left, time_index);
}

void dsp_rws_set_role(uint8_t role, uint8_t type)
{
    dsp_ipc_set_rws(role, type);
}

void dsp_remote_init(T_DSP_MGR_SESSION_HANDLE handle, uint8_t clk_ref, uint32_t timestamp,
                     bool sync_flag)
{
    T_DSP_MGR_SESSION *session;
    T_REMOTE_SESSION_STATE session_state;
    T_REMOTE_SESSION_ROLE session_role;
    uint8_t bt_clk_index;
    uint32_t sample_rate;
    uint8_t role = 1;

    session = (T_DSP_MGR_SESSION *)handle;

    if (bt_piconet_id_get((T_BT_CLK_REF)clk_ref, &bt_clk_index, &role) == false)
    {
        bt_clk_index = 0;
        DIPC_PRINT_WARN0("dsp_remote_init: bt_piconet_id_get fail");
    }
    session_role = remote_session_role_get();
    session_state = remote_session_state_get();

    sample_rate = sport_mgr_tx_sample_rate_get(session->sport_handle);

    DIPC_PRINT_TRACE4("dsp_remote_init: clk index %d, rws_state %d clk_ref %u link_role %u",
                      bt_clk_index, session_state, clk_ref, role);
    dsp_ipc_set_rws_init(bt_clk_index, sample_rate, session->type);

    if (session_role == REMOTE_SESSION_ROLE_PRIMARY &&
        session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (!sync_flag)
        {
            dsp_rws_set_role(RWS_ROLE_NONE, session->type);
        }
        else
        {
            dsp_rws_set_role(RWS_ROLE_SRC, session->type);
        }
    }
    else if (session_role == REMOTE_SESSION_ROLE_SECONDARY &&
             session_state == REMOTE_SESSION_STATE_CONNECTED)
    {
        if (!sync_flag)
        {
            dsp_rws_set_role(RWS_ROLE_NONE, session->type);
        }
        else
        {
            dsp_rws_set_role(RWS_ROLE_SNK, session->type);
        }
    }
    else// single
    {
        dsp_rws_set_role(RWS_ROLE_NONE, session->type);
    }
}

void dsp_rws_seamless(uint8_t role)
{
    dsp_ipc_set_rws_seamless(role);
}

uint32_t dsp_mailbox_process(T_DSP_SCHED_MSG *p_msg)
{
    T_DSP_MAILBOX_DATA param;

    switch (p_msg->subtype)
    {
    case MAILBOX_D2H_WATCHDOG_TIMEOUT:
    case MAILBOX_D2H_DSP_EXCEPTION:
        {

            DBG_DIRECT("dsp_mailbox_process: exception, subtype: 0x%02X, p_data: %p",
                       p_msg->subtype, p_msg->p_data);
            dsp_hal_reset();

            dsp_ipc_set_log_output_sel(sys_cfg_const.dsp_log_output_select);

            //if exception happen, it needs to reload algorithm according logic codec
            dsp_mgr_dsp_state_set(DSP_STATE_IDLE);

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_DSP_EXCEPTION, NULL);
            chip_reset(RESET_ALL);
        }
        break;

    case MAILBOX_D2H_DSP_ADCDAC_DATA0:
    case MAILBOX_D2H_DSP_ADCDAC_DATA1:
        {
            param.data_len = p_msg->data_len;
            param.p_data = p_msg->p_data;
            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_MAILBOX_DSP_DATA, &param);
        }
        break;

    default:
        break;
    }

    return 0;
}

void dsp_power_on(void)
{
    dsp_hal_reset();

    dsp_ipc_set_log_output_sel(sys_cfg_const.dsp_log_output_select);
}

static void dsp_mgr_wait_load_handler(void *handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    if (session)
    {
        dsp_send_msg(DSP_MSG_PREPARE_READY, 0, session, 0);
    }
}

void dsp_state_machine(uint8_t event, uint32_t parameter)
{
    T_DSP_STATE dsp_state;
    T_SYS_EVENT_GROUP_HANDLE event_group;

    dsp_state = dsp_mgr_dsp_state_get();
    event_group = dsp_mgr_event_group_get();

    DIPC_PRINT_TRACE3("dsp_state_machine: state 0x%02x, event 0x%02x, parameter 0x%08x",
                      dsp_state, event, parameter);

    switch (dsp_state)
    {
    case DSP_STATE_LOADER:
        {
            if (event == DSP_EVENT_IMAGE_LOAD_FINISH)
            {
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
                if (dsp_hal_ready_get() == READY_BIT_ALL)
                {
                    dsp_hal_enable_dsp2();
                }
#endif
                dsp_mgr_dsp_state_set(DSP_STATE_FW_READY);
                sys_event_flag_post(event_group,
                                    SYS_EVENT_FLAG_OPT_CLEAR,
                                    DSP_MGR_EVENT_FLAG_WAIT_LOAD);
            }
            else if (event == DSP_EVENT_START_FW)
            {
                T_DSP_MGR_SESSION *session;

                session = (T_DSP_MGR_SESSION *)parameter;
                if (session != NULL)
                {
                    sys_event_flag_wait(event_group,
                                        dsp_mgr_wait_load_handler,
                                        session,
                                        DSP_MGR_EVENT_FLAG_WAIT_LOAD,
                                        SYS_EVENT_FLAG_TYPE_CLEAR_AND);
                }
            }
        }
        break;

    case DSP_STATE_IDLE:
        {
            T_DSP_MGR_SESSION *session;

            session = NULL;

            if (event == DSP_EVENT_POWER_DOWN_DSP)
            {
                dsp_mgr_dsp_state_set(DSP_STATE_WAIT_OFF);
                dsp_ipc_set_power_down(0);
            }
            else if (event == DSP_EVENT_POWER_ON_DSP)
            {
                dsp_power_on();

                dsp_ipc_set_boot_config(1);

                dsp_mgr_dsp_state_set(DSP_STATE_IDLE);

                sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_POWER_ON, NULL);
            }
            else if (event == DSP_EVENT_START_FW)
            {
                session = (T_DSP_MGR_SESSION *)parameter;

                if (session != NULL)
                {
                    if (dsp_load_algorithm_code((T_DSP_SESSION_TYPE)session->type))
                    {
                        sys_event_flag_post(event_group,
                                            SYS_EVENT_FLAG_OPT_CLEAR,
                                            DSP_MGR_EVENT_FLAG_AUDIO_ALGO_PARAM);

                        sys_event_flag_post(event_group,
                                            SYS_EVENT_FLAG_OPT_CLEAR,
                                            DSP_MGR_EVENT_FLAG_VOICE_ALGO_PARAM);
                    }

                    dsp_mgr_dsp_state_set(DSP_STATE_LOADER);

                    sys_event_flag_post(event_group,
                                        SYS_EVENT_FLAG_OPT_SET,
                                        DSP_MGR_EVENT_FLAG_WAIT_LOAD);

                    sys_event_flag_wait(event_group,
                                        dsp_mgr_wait_load_handler,
                                        session,
                                        DSP_MGR_EVENT_FLAG_WAIT_LOAD,
                                        SYS_EVENT_FLAG_TYPE_CLEAR_AND);
                }
            }
        }
        break;

    case DSP_STATE_WAIT_OFF:
        {
            if (event == DSP_EVENT_D2H_POWER_DOWN_ACK)
            {

                dsp_hal_shut_down();
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-801*/
                shm2_disable();
#endif
                dsp_mgr_dsp_state_set(DSP_STATE_OFF);
            }
        }
        break;

    case DSP_STATE_OFF:
        {
            if (event == DSP_EVENT_POWER_ON_DSP)
            {
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-801*/
                shm2_enable();
#endif
                dsp_power_on();

                dsp_mgr_dsp_state_set(DSP_STATE_IDLE);

                sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_POWER_ON, NULL);
            }
        }
        break;

    case DSP_STATE_FW_READY:
        {
            if (event == DSP_EVENT_STOP_FW)
            {
                dsp_mgr_dsp_state_set(DSP_STATE_IDLE);
            }
        }
        break;

    default:
        break;
    }
}

static bool dsp_mgr_hal_cback(uint32_t event, void *msg)
{
    switch (event)
    {
    case DSP_HAL_EVT_DSP_LOAD_PART:
        {
            dsp_load_next_bin();
        }
        break;

    case DSP_HAL_EVT_DSP_LOAD_FINISH:
        {
            dsp_load_finish();
        }
        break;

    case DSP_HAL_EVT_CODEC_STATE:
        {
            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_CODEC_STATE, (void *)msg);
        }
        break;

    case DSP_HAL_EVT_FADE_OUT_FINISH:
        {
            T_SYS_EVENT_GROUP_HANDLE event_group;

            event_group = dsp_mgr_event_group_get();

            sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                DSP_MGR_EVENT_FLAG_FADE_OUT);
        }
        break;

    case DSP_HAL_EVT_MAILBOX:
        {
            T_DSP_SCHED_MSG *dsp_msg;
            dsp_msg = (T_DSP_SCHED_MSG *)msg;

            dsp_mailbox_process(dsp_msg);
        }
        break;

    case DSP_HAL_EVT_SPORT_STOP:
        {
            T_DSP_SCHED_MSG *dsp_msg;
            T_DSP_MGR_SESSION *session;

            dsp_msg = (T_DSP_SCHED_MSG *)msg;
            session = (T_DSP_MGR_SESSION *)dsp_msg->p_data;

            if (session)
            {
                session->state = DSP_SESSION_STATE_IDLE;

                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_SPORT_STOP_FAKE, 0);
                }

                if (dsp_mgr_is_idle())
                {
                    dsp_state_machine(DSP_EVENT_STOP_FW, 0);
                }
            }

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_SPORT_STOP_FAKE, (void *)msg);
        }
        break;

    case DSP_HAL_EVT_SPORT_START:
        {
            T_DSP_SCHED_MSG *dsp_msg;
            T_DSP_MGR_SESSION *session;

            dsp_msg = (T_DSP_SCHED_MSG *)msg;
            session = (T_DSP_MGR_SESSION *)dsp_msg->p_data;

            if (session)
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_SPORT_START_FAKE, 0);
                }
            }
        }
        break;

    case DSP_HAL_EVT_PREPARE_READY:
        {
            T_DSP_SCHED_MSG *dsp_msg;
            T_DSP_MGR_SESSION *session;

            dsp_msg = (T_DSP_SCHED_MSG *)msg;
            session = (T_DSP_MGR_SESSION *)dsp_msg->p_data;

            if (session)
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_PREPARE_READY, 0);
                }
            }
        }
        break;

    case DSP_HAL_EVT_FAKE_ACTION:
        {
            T_DSP_SCHED_MSG *dsp_msg;

            dsp_msg = (T_DSP_SCHED_MSG *)msg;

            if (dsp_msg->subtype == DSP_MSG_SUB_TYPE_NOTIFICATION_END)
            {
                dsp_ipc_event_handler(DSP_IPC_EVT_NOTIFICATION_FINISH, 0);
            }
        }
        break;
    }

    return true;
}

static void dsp_mgr_gate_status_handler(T_DIPC_GATE_STATUS *gate_status)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;
    uint32_t wait_flag;

    event_group = dsp_mgr_event_group_get();
    wait_flag = 0;

    if (gate_status->ready == true)
    {
        if (gate_status->dir_bit & BIT(DIPC_DIRECTION_TX))
        {
            dsp_mgr_sub_state_set(gate_status->gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_RUNNING);
            wait_flag |= gate_wait_tx_ready_flag[gate_status->gate_id];

            if (gate_status->gate_id == DIPC_GATE_ID0)
            {
                dsp_mgr_db.decoder.busy = true;
            }
        }

        if (gate_status->dir_bit & BIT(DIPC_DIRECTION_RX))
        {
            dsp_mgr_sub_state_set(gate_status->gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_RUNNING);
            wait_flag |= gate_wait_rx_ready_flag[gate_status->gate_id];

            if (gate_status->gate_id == DIPC_GATE_ID0)
            {
                dsp_mgr_db.encoder.busy = true;
            }
        }

        sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_CLEAR, wait_flag);
    }
    else if (gate_status->ready == false)
    {
        if (gate_status->dir_bit & BIT(DIPC_DIRECTION_TX))
        {
            dsp_mgr_sub_state_set(gate_status->gate_id, DIPC_DIRECTION_TX, DSP_SUBSTATE_IDLE);
            wait_flag |= gate_wait_tx_stop_flag[gate_status->gate_id];

            if (gate_status->gate_id == DIPC_GATE_ID0)
            {
                dsp_mgr_db.decoder.busy = false;
                dsp_mgr_db.decoder.owner = NULL;
            }

            sys_event_flag_post(event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                gate_sig_proc_flag[gate_status->gate_id]);
        }

        if (gate_status->dir_bit & BIT(DIPC_DIRECTION_RX))
        {
            dsp_mgr_sub_state_set(gate_status->gate_id, DIPC_DIRECTION_RX, DSP_SUBSTATE_IDLE);
            wait_flag |= gate_wait_rx_stop_flag[gate_status->gate_id];

            if (gate_status->gate_id == DIPC_GATE_ID0)
            {
                dsp_mgr_db.encoder.busy = false;
                dsp_mgr_db.encoder.owner = NULL;
            }
        }

        sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_CLEAR, wait_flag);
    }
}

static void dsp_request_clk(uint32_t require_mips)
{
    uint32_t real_mips;

    int32_t ret = pm_dsp1_freq_set(require_mips, &real_mips);
    if (ret != 0)
    {
        AUDIO_PRINT_ERROR3("dsp_request_clk fail %x, require_mips %d, realmips %d", ret, require_mips,
                           real_mips);
    }
    dsp_ipc_set_dsp_config(real_mips, true);
}

#if (TARGET_RTL8773D == 1)
static void dsp2_request_clk(uint32_t require_mips)
{
    uint32_t real_mips;
    // todo optimize in bblite2
    extern int32_t pm_dsp2_freq_set(uint32_t required_mhz, uint32_t *actual_mhz);
    int32_t ret = pm_dsp2_freq_set(require_mips, &real_mips);
    if (ret != 0)
    {
        AUDIO_PRINT_ERROR3("dsp2_request_clk fail %x, require_mips %d, realmips %d", ret, require_mips,
                           real_mips);
    }
    dsp_ipc_set_dsp2_config(real_mips, true);
}
#endif

static bool dsp_ipc_event_handler(T_DSP_IPC_EVENT event, uint32_t param)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;

    T_DSP_MGR_SESSION *session;

    session = dsp_mgr_db.decoder.owner;

    if (!os_queue_search(&dsp_mgr_db.session_queue, session))
    {
        session = NULL;
    }

    if (event != DSP_IPC_EVT_CODEC_PIPE_DATA_ACK &&
        event != DSP_IPC_EVT_CODEC_PIPE_DATA_IND)
    {
        DIPC_PRINT_TRACE3("dsp_ipc_event_handler: event 0x%02x, param 0x%08x, session %p",
                          event, param, session);
    }

    event_group = dsp_mgr_event_group_get();

    switch (event)
    {
    case DSP_IPC_EVT_FADE_OUT_FINISH:
        {
            sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                DSP_MGR_EVENT_FLAG_FADE_OUT);
        }
        break;

    case DSP_IPC_EVT_POWER_OFF_ACK:
        {
            sys_event_flag_post(event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                DSP_MGR_EVENT_FLAG_AUDIO_ALGO_PARAM);

            sys_event_flag_post(event_group,
                                SYS_EVENT_FLAG_OPT_CLEAR,
                                DSP_MGR_EVENT_FLAG_VOICE_ALGO_PARAM);

            dsp_state_machine(DSP_EVENT_D2H_POWER_DOWN_ACK, 0);

            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_POWER_OFF, NULL);
        }
        break;

    case DSP_IPC_EVT_CLK_REQUEST:
        {
            dsp_request_clk(param);
        }
        break;

#if (TARGET_RTL8773D == 1)
    case DSP_IPC_EVT_DSP2_CLK_REQUEST:
        {
            dsp2_request_clk(param);
        }
        break;
#endif

    case DSP_IPC_EVT_NOTIFICATION_FINISH:
        {
            sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                DSP_MGR_EVENT_FLAG_WAIT_NOTIFICATION_FINISH);
            sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_CLEAR,
                                DSP_MGR_EVENT_FLAG_NOTIFICATION_START);
        }
        break;

    case DSP_IPC_EVT_AUDIOPLAY_VOLUME_INFO:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_AUDIOPLAY_VOLUME_INFO, param);
            }
        }
        break;

    case DSP_IPC_EVT_LATENCY_REPORT:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_LATENCY_REPORT, param);
            }
        }
        break;

    case DSP_IPC_EVT_PLC_NUM:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_PLC_NUM, param);
            }
        }
        break;

    case DSP_IPC_EVT_DECODER_PLC_NOTIFY:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DECODER_PLC_NOTIFY, param);
            }
        }
        break;

    case DSP_IPC_EVT_DECODE_EMPTY:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DECODE_EMPTY, 0);
            }
        }
        break;

    case DSP_IPC_EVT_DSP_SYNC_V2_SUCC:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DSP_SYNC_V2_SUCC, 0);
            }
        }
        break;

    case DSP_IPC_EVT_DSP_UNSYNC:
    case DSP_IPC_EVT_BTCLK_EXPIRED:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DSP_UNSYNC, 0);
            }
        }
        break;

    case DSP_IPC_EVT_DSP_SYNC_UNLOCK:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DSP_SYNC_UNLOCK, 0);
            }
        }
        break;

    case DSP_IPC_EVT_DSP_SYNC_LOCK:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DSP_SYNC_LOCK, 0);
            }
        }
        break;

    case DSP_IPC_EVT_SYNC_EMPTY:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_SYNC_EMPTY, 0);
            }
        }
        break;

    case DSP_IPC_EVT_SYNC_LOSE_TIMESTAMP:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_SYNC_LOSE_TIMESTAMP, 0);
            }
        }
        break;

    case DSP_IPC_EVT_JOIN_CLK:
        {
            if ((session != NULL) && (session->callback != NULL))
            {
                session->callback(session->context, DSP_MGR_EVT_DSP_JOIN_INFO, param);
            }
        }
        break;

    case DSP_IPC_EVT_GATE_STATUS:
        {
            T_DIPC_GATE_STATUS *gate_status;

            gate_status = (T_DIPC_GATE_STATUS *)param;

            dsp_mgr_gate_status_handler(gate_status);
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_CREATE:
        {
            T_DIPC_CODEC_PIPE_CREATE_CMPL *p_info;

            p_info = (T_DIPC_CODEC_PIPE_CREATE_CMPL *)param;
            session = (T_DSP_MGR_SESSION *)p_info->session_id;

            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    if (p_info->status == DIPC_ERROR_SUCCESS)
                    {
                        session->src_transport_address = p_info->src_transport_address;
                        session->snk_transport_address = p_info->snk_transport_address;
                    }

                    session->callback(session->context, DSP_MGR_EVT_CODEC_PIPE_CREATE, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_DESTROY:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_CODEC_PIPE_DESTROY, param);
                }
            }

            if (dsp_mgr_is_idle())
            {
                dsp_state_machine(DSP_EVENT_STOP_FW, 0);
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_START:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_CODEC_PIPE_START, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_STOP:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_CODEC_PIPE_STOP, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_DATA_ACK:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_REQ_DATA, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_DATA_IND:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_DATA_IND, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_PRE_MIXER_ADD:
    case DSP_IPC_EVT_CODEC_PIPE_POST_MIXER_ADD:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_CODEC_PIPE_MIXED, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_CODEC_PIPE_MIXER_REMOVE:
        {
            session = (T_DSP_MGR_SESSION *)param;
            if (os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                if (session->callback != NULL)
                {
                    session->callback(session->context, DSP_MGR_EVT_CODEC_PIPE_DEMIXED, param);
                }
            }
        }
        break;

    case DSP_IPC_EVT_OPEN_AIR_AVC:
        {
            sys_ipc_publish("dsp_mgr", DSP_MGR_EVT_OPEN_AIR_AVC, (void *)param);
        }
        break;

    default:
        break;
    }

    return true;
}

void dsp_cfg_comm_setting(uint32_t scenario)
{
    dsp_ipc_set_dsp_config(0x0028, false);
    dsp_ipc_init_dsp_sdk(false, (uint8_t)scenario);
}

static void dsp_download_algo_param(T_DSP_MGR_SESSION *session)
{
    uint16_t i;
    uint32_t flag;
    uint8_t *cmd_buffer;
    uint16_t algo_offset;
    uint8_t scenario;
    uint16_t algo_cmd_length;
    T_SYS_EVENT_GROUP_HANDLE event_group;

    if (session->type == DSP_SESSION_TYPE_VOICE)
    {
        flag = DSP_MGR_EVENT_FLAG_VOICE_ALGO_PARAM;
        scenario = DSP_APP_VOICE;
    }
    else
    {
        flag = DSP_MGR_EVENT_FLAG_AUDIO_ALGO_PARAM;
        scenario = DSP_APP_A2DP;
    }

    event_group = dsp_mgr_event_group_get();
    if (sys_event_flag_check(event_group, flag) == true)
    {
        return ;
    }

    sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_SET, flag);

    for (i = 0; i < dsp_mgr_db.algo_block.algo_num; i++)
    {
        algo_offset = dsp_mgr_db.algo_block_offset + dsp_mgr_db.algo_block.param_header[i].offset;
        algo_cmd_length = dsp_mgr_db.algo_block.param_header[i].cmd_length;

        cmd_buffer = os_mem_alloc2(algo_cmd_length);
        if (cmd_buffer != NULL)
        {
            if (((dsp_mgr_db.algo_block.param_header[i].scenario == scenario) ||
                 (dsp_mgr_db.algo_block.param_header[i].scenario == SHM_SCENARIO_ALL)))
            {
                fmc_flash_nor_read(flash_dsp_cfg_addr_get() +
                                   DSP_PARAM_OFFSET + algo_offset,
                                   cmd_buffer, algo_cmd_length);

                dsp_ipc_download_algo_param(cmd_buffer, algo_cmd_length);

            }
            os_mem_free(cmd_buffer);
        }
    }
}

T_DSP_STATE dsp_mgr_get_state(void)
{
    return dsp_mgr_dsp_state_get();
}

T_SYS_IPC_HANDLE dsp_mgr_register_cback(P_SYS_IPC_CBACK cback)
{
    if (cback == NULL)
    {
        return NULL;
    }

    return sys_ipc_subscribe("dsp_mgr", cback);
}

void dsp_mgr_unregister_cback(T_SYS_IPC_HANDLE handle)
{
    if (handle == NULL)
    {
        return ;
    }
    sys_ipc_unsubscribe(handle);
}

static bool dsp_mgr_io_fliter_all(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    return true;
}

static bool dsp_mgr_io_fliter_rx(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    return ((physical_path->sport_ch_dir == AUDIO_ROUTE_SPORT_CH_DIR_RX) ? true : false);
}

static bool dsp_mgr_io_fliter_tx(T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path)
{
    return ((physical_path->sport_ch_dir == AUDIO_ROUTE_SPORT_CH_DIR_TX) ? true : false);
}

static bool dsp_mgr_send_action(T_AUDIO_CATEGORY category, T_ACTION_CONFIG action,
                                P_IO_FLITER_CBACK fliter, P_ACTION_CBACK cback)
{
    uint8_t tlv_cnt;
    uint8_t i;
    T_DSP_IPC_LOGIC_IO logic_io;
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP pysical_path_group;

    pysical_path_group = audio_route_physical_path_take(category);

    if (pysical_path_group.physical_path_num == 0)
    {
        return false;
    }

    tlv_cnt = 0;

    logic_io.version = 0;
    logic_io.rsvd = 0;
    logic_io.tlv = os_mem_zalloc2(sizeof(T_DSP_IPC_LOGIC_IO_TLV) *
                                  pysical_path_group.physical_path_num);

    if (logic_io.tlv != NULL)
    {
        for (i = 0; i < pysical_path_group.physical_path_num; ++i)
        {
            if (fliter(&(pysical_path_group.physical_path[i])))
            {
                logic_io.tlv[tlv_cnt].category = category;
                logic_io.tlv[tlv_cnt].logic_io_id = pysical_path_group.physical_path[i].logic_io_type;
                logic_io.tlv[tlv_cnt].sport_id = pysical_path_group.physical_path[i].sport_idx;
                logic_io.tlv[tlv_cnt].rtx = pysical_path_group.physical_path[i].sport_ch_dir;
                logic_io.tlv[tlv_cnt].channel = pysical_path_group.physical_path[i].sport_ch;
                logic_io.tlv[tlv_cnt].polarity = pysical_path_group.physical_path[i].polarity;

                ++tlv_cnt;
            }
        }
        logic_io.tlv_cnt = tlv_cnt;

        cback(action, &logic_io);

        os_mem_free(logic_io.tlv);
    }

    audio_route_physical_path_give(&pysical_path_group);

    return true;
}

bool dsp_mgr_apt_action(T_DSP_IPC_ACTION action)
{
    T_ACTION_CONFIG action_config;

    memset(&action_config, 0, sizeof(T_ACTION_CONFIG));
    action_config.action = action;
    if (action == DSP_IPC_ACTION_START)
    {
        action_config.fade_action = FADE_ACTION_IN;
    }
    else
    {
        action_config.fade_action = FADE_ACTION_OUT;
    }

    return dsp_mgr_send_action(AUDIO_CATEGORY_APT, action_config, dsp_mgr_io_fliter_all,
                               dsp_ipc_apt_action);
}

bool dsp_mgr_decoder_action(T_AUDIO_CATEGORY category, T_DSP_IPC_ACTION action)
{
    T_ACTION_CONFIG action_config;

    memset(&action_config, 0, sizeof(T_ACTION_CONFIG));
    action_config.action = action;

    return dsp_mgr_send_action(category, action_config, dsp_mgr_io_fliter_tx, dsp_ipc_decoder_action);
}

bool dsp_mgr_encoder_action(T_AUDIO_CATEGORY category, T_DSP_IPC_ACTION action)
{
    T_ACTION_CONFIG action_config;

    memset(&action_config, 0, sizeof(T_ACTION_CONFIG));
    action_config.action = action;

    return dsp_mgr_send_action(category, action_config, dsp_mgr_io_fliter_rx, dsp_ipc_encoder_action);
}

bool dsp_mgr_vad_action(T_DSP_IPC_ACTION action)
{
    T_ACTION_CONFIG action_config;

    memset(&action_config, 0, sizeof(T_ACTION_CONFIG));
    action_config.action = action;

    if (action == DSP_IPC_ACTION_START)
    {
        action_config.vad_hw = true;
        action_config.vad_kws =  true;
    }
    else
    {
        action_config.vad_hw = false;
        action_config.vad_kws =  false;
    }

    return dsp_mgr_send_action(AUDIO_CATEGORY_VAD, action_config, dsp_mgr_io_fliter_all,
                               dsp_ipc_vad_action);
}

bool dsp_mgr_line_in_action(T_DSP_IPC_ACTION action)
{
    T_ACTION_CONFIG action_config;

    memset(&action_config, 0, sizeof(T_ACTION_CONFIG));
    action_config.action = action;

    return dsp_mgr_send_action(AUDIO_CATEGORY_ANALOG, action_config, dsp_mgr_io_fliter_all,
                               dsp_ipc_line_in_action);
}

bool dsp_mgr_hal_data_cb(uint32_t event, void *msg)
{
    T_DSP_MGR_SESSION *session;

    session = NULL;

    switch (event)
    {
    case DSP_HAL_EVT_DATA_IND:
        {
            session = (T_DSP_MGR_SESSION *)dsp_mgr_db.encoder.owner;

            if (session != NULL)
            {
                session->callback(session->context, DSP_MGR_EVT_DATA_IND, 0);
            }
            else
            {
                d2h_data_flush();
            }
        }
        break;

    case DSP_HAL_EVT_DATA_ACK:
        {
            session = (T_DSP_MGR_SESSION *)dsp_mgr_db.decoder.owner;

            if (!os_queue_search(&dsp_mgr_db.session_queue, session))
            {
                session = NULL;
            }

            if (session != NULL)
            {
                session->callback(session->context, DSP_MGR_EVT_REQ_DATA, 0);
            }
        }
        break;
    }

    return true;
}

bool dsp_mgr_init(void)
{
    int32_t ret = 0;

    memset(&dsp_mgr_db, 0, sizeof(T_DSP_MGR_DB));

    power_check_cb_register(dsp_lps_check);

    if (dsp_hal_init() == false)
    {
        ret = 1;
        goto fail_init_hal;
    }
    dsp_hal_register_cback(DSP_HAL_PRIORITY_HIGH, dsp_mgr_hal_data_cb);

    if (dsp_ipc_init() == false)
    {
        ret = 2;
        goto fail_init_ipc;
    }

    if (dsp_load_init() != true)
    {
        ret = 3;
        goto fail_init_load;
    }

    dsp_param_init();

    dsp_mgr_db.event = dsp_hal_register_cback(DSP_HAL_PRIORITY_LOW, dsp_mgr_hal_cback);

    if (dsp_mgr_db.event == NULL)
    {
        ret = 4;
        goto fail_register_cback;
    }

    dsp_mgr_db.event_group = sys_event_group_create(0);

    os_queue_init(&(dsp_mgr_db.session_queue));

    dsp_ipc_cback_register(dsp_ipc_event_handler);

    dsp_mgr_db.dsp_state = DSP_STATE_OFF;

    return true;

fail_register_cback:
fail_init_load:
    dsp_ipc_deinit();
fail_init_ipc:
    dsp_hal_deinit();
fail_init_hal:
    DIPC_PRINT_ERROR1("dsp_mgr_init: failed %d", -ret);
    return false;
}

void dsp_mgr_deinit(void)
{
    dsp_hal_unregister_cback(dsp_mgr_db.event);
    sys_event_group_delete(dsp_mgr_db.event_group);

    T_DSP_MGR_SESSION *session = os_queue_out(&(dsp_mgr_db.session_queue));
    while (session != NULL)
    {
        os_mem_free(session);
        session = os_queue_out(&(dsp_mgr_db.session_queue));
    }

    dsp_load_deinit();

    dsp_ipc_deinit();

    dsp_hal_deinit();
}

static bool dsp_mgr_gate_is_stable(void)
{
    T_DSP_SUBSTATE ad_state;
    T_DSP_SUBSTATE da_state;
    uint8_t i;

    for (i = 0; i < DIPC_GATE_MAX; ++i)
    {
        ad_state = dsp_mgr_sub_state_get(i, DIPC_DIRECTION_RX);
        da_state = dsp_mgr_sub_state_get(i, DIPC_DIRECTION_TX);

        DIPC_PRINT_TRACE3("dsp_mgr_gate_is_stable: id %d, ad_state %d, da_state %d",
                          i,  ad_state, da_state);

        if (((ad_state == DSP_SUBSTATE_STARTING) ||
             (ad_state == DSP_SUBSTATE_STOPPING)) &&
            ((da_state == DSP_SUBSTATE_STARTING) ||
             (da_state == DSP_SUBSTATE_STOPPING)))
        {
            return false;
        }
    }

    return true;
}

bool dsp_mgr_is_stable(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;
    T_DSP_STATE dsp_state;
    T_DSP_MGR_SESSION *session;
    bool gate_stable;
    bool need_initial;
    bool in_initial;
    bool fading;
    bool loader_match;

    loader_match = true;

    gate_stable = dsp_mgr_gate_is_stable();
    dsp_state = dsp_mgr_dsp_state_get();
    need_initial = dsp_load_need_initial();
    in_initial = dsp_load_in_initial();
    event_group = dsp_mgr_event_group_get();
    fading = sys_event_flag_check(event_group, DSP_MGR_EVENT_FLAG_FADE_OUT);

    if (dsp_state == DSP_STATE_FW_READY)
    {
        if (handle != NULL)
        {
            session = (T_DSP_MGR_SESSION *)handle;
            loader_match = dsp_loader_bin_match((T_DSP_SESSION_TYPE)session->type);
        }
    }

    DIPC_PRINT_TRACE6("dsp_mgr_is_stable: gate_stable %d, dsp_state %d, need_initial %d, in_initial %d, fading %d, loader_match %d",
                      gate_stable, dsp_state, need_initial, in_initial, fading, loader_match);

    if (need_initial == true)
    {
        dsp_mgr_start();
        return false;
    }

    if ((gate_stable == false) ||
        (in_initial == true) ||
        (dsp_state == DSP_STATE_WAIT_OFF) ||
        (dsp_state == DSP_STATE_LOADER) ||
        (fading == true) ||
        (loader_match == false))
    {

        return false;
    }
    else
    {
        return true;
    }
}

static uint8_t dsp_translate_aac_type(T_AUDIO_AAC_TRANSPORT_FORMAT format)
{
    uint8_t index;

    index = AAC_TYPE_RAW;

    /* 0x0: AAC_TYPE_LATM_NORMAL
             0x1: AAC_TYPE_LATM_SIMPLE
             0x2: AAC_TYPE_ADTS
             0x3: AAC_TYPE_ADIF
             0x4: AAC_TYPE_RAW */

    switch (format)
    {
    case AUDIO_AAC_TRANSPORT_FORMAT_LATM:
        {
            index = AAC_TYPE_LATM_NORMAL;
        }
        break;

    case AUDIO_AAC_TRANSPORT_FORMAT_ADTS:
        {
            index = AAC_TYPE_ADTS;
        }
        break;

    case AUDIO_AAC_TRANSPORT_FORMAT_ADIF:
        {
            index = AAC_TYPE_ADIF;
        }
        break;

    default:
        break;
    }

    return index;
}

static bool audio_decode_set(T_DSP_MGR_SESSION *session, T_AUDIO_FORMAT_INFO *format_info)
{
    T_DSP_IPC_DECODER *decoder_cfg;
    T_AUDIO_FORMAT_TYPE format_type;

    session->decoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_DECODER));

    if (session->decoder_param == NULL)
    {
        return false;
    }

    decoder_cfg = session->decoder_param;
    format_type = format_info->type;

    switch (format_type)
    {
    case AUDIO_FORMAT_TYPE_SBC:
        {
            T_AUDIO_SBC_ATTR *sbc_attr;
            sbc_attr = &(format_info->attr.sbc);

            decoder_cfg->algorithm = ALGORITHM_SBC;
            decoder_cfg->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder_cfg->chann_mode = (sbc_attr->chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO) ?
                                      STREAM_CHANNEL_OUTPUT_MONO : STREAM_CHANNEL_OUTPUT_STEREO;
            decoder_cfg->sample_rate = sbc_attr->sample_rate;
            decoder_cfg->bit_res = 0;
            decoder_cfg->samples_per_frame = (sbc_attr->block_length * sbc_attr->subband_num *
                                              (decoder_cfg->chann_mode + 1));
        }
        break;

    case AUDIO_FORMAT_TYPE_AAC:
        {
            T_AUDIO_AAC_ATTR *aac_attr;
            aac_attr = &(format_info->attr.aac);

            decoder_cfg->algorithm = ALGORITHM_AAC;
            decoder_cfg->sub_type = dsp_translate_aac_type(aac_attr->transport_format);
            decoder_cfg->chann_mode = aac_attr->chann_num - 1;
            decoder_cfg->sample_rate = aac_attr->sample_rate;
            decoder_cfg->bit_res = 0;
            decoder_cfg->samples_per_frame = AACLC_FRAME_SIZE;
        }
        break;

    case AUDIO_FORMAT_TYPE_PCM:
        {
            T_AUDIO_PCM_ATTR *pcm_attr;
            pcm_attr = &(format_info->attr.pcm);
            decoder_cfg->algorithm = ALGORITHM_USB_AUDIO;
            decoder_cfg->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder_cfg->chann_mode = (pcm_attr->chann_num == 1 ? STREAM_CHANNEL_OUTPUT_MONO :
                                       STREAM_CHANNEL_OUTPUT_STEREO);
            decoder_cfg->sample_rate = pcm_attr->sample_rate;
            decoder_cfg->samples_per_frame = pcm_attr->frame_length / pcm_attr->chann_num /
                                             pcm_attr->bit_width * 8;
            switch (pcm_attr->bit_width)
            {
            case 16:
                {
                    decoder_cfg->bit_res = 0;
                }
                break;

            case 24:
                {
                    decoder_cfg->bit_res = 1;
                }
                break;

            default:
                {
                    decoder_cfg->bit_res = 0;
                }
                break;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr = &(format_info->attr.lc3);
            decoder_cfg->algorithm = ALGORITHM_LC3;
            decoder_cfg->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder_cfg->sample_rate = lc3_attr->sample_rate;

            decoder_cfg->comm_header.decode_algorithm = ALGORITHM_LC3;
            decoder_cfg->comm_header.sample_rate = lc3_attr->sample_rate;
            decoder_cfg->comm_header.frame_size_per_ch = audio_codec_frame_size_get(format_type, lc3_attr);
            decoder_cfg->comm_header.channel_num_per_block = audio_codec_chann_num_get(format_type, lc3_attr);
            decoder_cfg->comm_header.bit_width = 0;


            decoder_cfg->decoder.frame_duration = lc3_attr->frame_duration;
            decoder_cfg->decoder.blocks_per_sdu = 1;
            decoder_cfg->decoder.channel_allocation = lc3_attr->chann_location;
            decoder_cfg->decoder.octets_per_frame = lc3_attr->frame_length;
            decoder_cfg->decoder.max_transport_latency_ms = 50;//deprecated
            decoder_cfg->decoder.sdu_interval = audio_codec_frame_duration_get(format_type, lc3_attr);
            decoder_cfg->decoder.presentation_delay_us = lc3_attr->presentation_delay;
            decoder_cfg->decoder.retransmission_num = 0;//deprecated
            decoder_cfg->decoder.framed_unframed = 0;//deprecated
            decoder_cfg->decoder.bis_cis = 0;//deprecated
            decoder_cfg->decoder.plc_method = 0;//deprecated
        }
        break;

    case AUDIO_FORMAT_TYPE_MP3:
        {
            T_AUDIO_MP3_ATTR *mp3_attr;

            mp3_attr = &(format_info->attr.mp3);

            decoder_cfg->algorithm = ALGORITHM_MP3;
            decoder_cfg->sub_type = AAC_TYPE_ADTS;
            decoder_cfg->samples_per_frame = MP3_FRAME_SIZE;
            decoder_cfg->chann_mode = (mp3_attr->chann_mode == AUDIO_MP3_CHANNEL_MODE_MONO) ?
                                      STREAM_CHANNEL_OUTPUT_MONO : STREAM_CHANNEL_OUTPUT_STEREO;
            decoder_cfg->sample_rate = mp3_attr->sample_rate;
        }
        break;

    case AUDIO_FORMAT_TYPE_LDAC:
        {
            T_AUDIO_LDAC_ATTR *ldac_attr;

            ldac_attr = &(format_info->attr.ldac);
            decoder_cfg->algorithm = ALGORITHM_LDAC;
            decoder_cfg->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder_cfg->chann_mode = (ldac_attr->chann_mode == AUDIO_LDAC_CHANNEL_MODE_MONO) ?
                                      STREAM_CHANNEL_OUTPUT_MONO : STREAM_CHANNEL_OUTPUT_STEREO;
            decoder_cfg->sample_rate = ldac_attr->sample_rate;
            decoder_cfg->bit_res = 1;
            decoder_cfg->samples_per_frame = audio_codec_frame_size_get(format_type, ldac_attr);
        }
        break;

    case AUDIO_FORMAT_TYPE_LHDC:
        {
            T_AUDIO_LHDC_ATTR *lhdc_attr;

            lhdc_attr = &(format_info->attr.lhdc);
            decoder_cfg->algorithm = ALGORITHM_LHDC;
            decoder_cfg->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder_cfg->chann_mode = (lhdc_attr->chann_num == 1 ? STREAM_CHANNEL_OUTPUT_MONO :
                                       STREAM_CHANNEL_OUTPUT_STEREO);
            decoder_cfg->sample_rate = lhdc_attr->sample_rate;
            decoder_cfg->samples_per_frame = audio_codec_frame_size_get(format_type, lhdc_attr);
            switch (lhdc_attr->bit_width)
            {
            case 16:
                {
                    decoder_cfg->bit_res = 0;
                }
                break;

            case 24:
                {
                    decoder_cfg->bit_res = 1;
                }
                break;

            default:
                {
                    decoder_cfg->bit_res = 0;
                }
                break;
            }
        }
        break;

    default:
        break;
    }

    return true;
}

static bool voice_encode_decode_set(T_DSP_MGR_SESSION *session, T_AUDIO_FORMAT_INFO *format_info)
{
    T_DSP_IPC_ENCODER *encoder;
    T_DSP_IPC_DECODER *decoder;
    T_AUDIO_FORMAT_TYPE format_type;

    session->decoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_DECODER));

    if (session->decoder_param == NULL)
    {
        return false;
    }

    session->encoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_ENCODER));

    if (session->encoder_param == NULL)
    {
        os_mem_free(session->decoder_param);
        return false;
    }

    decoder = session->decoder_param;
    encoder = session->encoder_param;

    format_type = format_info->type;

    switch (format_type)
    {
    case AUDIO_FORMAT_TYPE_MSBC:
        {
            T_AUDIO_MSBC_ATTR *msbc_attr;

            msbc_attr = &(format_info->attr.msbc);

            decoder->algorithm = ALGORITHM_MSBC;
            decoder->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
            decoder->sample_rate = 16000;
            decoder->bit_res = 0;
            decoder->samples_per_frame = (msbc_attr->block_length * msbc_attr->subband_num *
                                          (decoder->chann_mode + 1));

            encoder->algorithm = ALGORITHM_MSBC;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
            encoder->sample_rate = 16000;
            encoder->bit_res = 0;
            encoder->samples_per_frame = (msbc_attr->block_length * msbc_attr->subband_num *
                                          (encoder->chann_mode + 1));
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            T_AUDIO_CVSD_ATTR *cvsd_attr;

            cvsd_attr = &(format_info->attr.cvsd);

            decoder->algorithm = ALGORITHM_CVSD;
            decoder->sub_type = AAC_TYPE_LATM_NORMAL;
            decoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
            decoder->sample_rate = cvsd_attr->sample_rate;
            decoder->bit_res = 0;
            decoder->samples_per_frame = audio_codec_frame_size_get(format_type, cvsd_attr);

            encoder->algorithm = ALGORITHM_CVSD;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
            encoder->sample_rate = cvsd_attr->sample_rate;
            encoder->bit_res = 0;
            encoder->samples_per_frame = audio_codec_frame_size_get(format_type, cvsd_attr);
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr = &(format_info->attr.lc3);

            decoder->algorithm = ALGORITHM_LC3_VOICE;
            decoder->sub_type = AAC_TYPE_LATM_NORMAL;

            decoder->sample_rate = lc3_attr->sample_rate;
            decoder->comm_header.decode_algorithm = ALGORITHM_LC3_VOICE;
            decoder->comm_header.sample_rate = lc3_attr->sample_rate;
            decoder->comm_header.frame_size_per_ch = audio_codec_frame_size_get(format_type, lc3_attr);
            decoder->comm_header.channel_num_per_block = audio_codec_chann_num_get(format_type, lc3_attr);
            decoder->comm_header.bit_width = 0;

            decoder->decoder.frame_duration = lc3_attr->frame_duration;
            decoder->decoder.blocks_per_sdu = 1;
            decoder->decoder.channel_allocation = lc3_attr->chann_location;
            decoder->decoder.octets_per_frame = lc3_attr->frame_length;
            decoder->decoder.max_transport_latency_ms = 50;//deprecated
            decoder->decoder.sdu_interval = audio_codec_frame_duration_get(format_type, lc3_attr);
            decoder->decoder.presentation_delay_us = lc3_attr->presentation_delay;
            decoder->decoder.retransmission_num = 0;//deprecated
            decoder->decoder.framed_unframed = 0;//deprecated
            decoder->decoder.bis_cis = 0;//deprecated
            decoder->decoder.plc_method = 0;//deprecated

            encoder->algorithm = ALGORITHM_LC3_VOICE;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;

            encoder->sample_rate = lc3_attr->sample_rate;
            encoder->comm_header.encode_algorithm = ALGORITHM_LC3_VOICE;
            encoder->comm_header.sample_rate =  lc3_attr->sample_rate;
            encoder->comm_header.frame_size_per_ch = audio_codec_frame_size_get(format_type, lc3_attr);
            encoder->comm_header.channel_num_per_block = audio_codec_chann_num_get(format_type, lc3_attr);
            encoder->comm_header.bit_width = 0;


            encoder->encoder.frame_duration = lc3_attr->frame_duration;
            encoder->encoder.blocks_per_sdu = 1;
            encoder->encoder.channel_allocation = lc3_attr->chann_location;
            encoder->encoder.octets_per_frame = lc3_attr->frame_length;
            encoder->encoder.max_transport_latency_ms = 50;
            encoder->encoder.sdu_interval = audio_codec_frame_duration_get(format_type, lc3_attr);
            encoder->encoder.presentation_delay_us = lc3_attr->presentation_delay;
            encoder->encoder.retransmission_num = 0;
            encoder->encoder.framed_unframed = 0;
            encoder->encoder.bis_cis = 0;
            encoder->encoder.encode_packet_format = 0;
        }
        break;
    }

    return true;
}

static bool record_encode_set(T_DSP_MGR_SESSION *session, T_AUDIO_FORMAT_INFO *format_info)
{

    T_DSP_IPC_ENCODER *encoder;
    T_DSP_SBC_ENCODE_PARAM *sbc_param;
    T_AUDIO_FORMAT_TYPE format_type;

    session->encoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_ENCODER));

    if (session->encoder_param == NULL)
    {
        return false;
    }

    session->sbc_param = os_mem_zalloc2(sizeof(T_DSP_SBC_ENCODE_PARAM));

    if (session->sbc_param == NULL)
    {
        os_mem_free(session->encoder_param);
        return false;
    }

    sbc_param = session->sbc_param;
    encoder = session->encoder_param;
    format_type = format_info->type;

    switch (format_type)
    {
    case AUDIO_FORMAT_TYPE_OPUS:
        {
            T_AUDIO_OPUS_ATTR *opus_attr;
            opus_attr = &(format_info->attr.opus);

            encoder->algorithm = ALGORITHM_OPUS_AUDIO;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = (opus_attr->chann_num == 1 ? STREAM_CHANNEL_OUTPUT_MONO :
                                   STREAM_CHANNEL_OUTPUT_STEREO);
            encoder->sample_rate = opus_attr->sample_rate;
            encoder->bit_res = 0;
            encoder->samples_per_frame = audio_codec_frame_size_get(format_type, opus_attr);
            encoder->opus_bitrate = opus_attr->bitrate;
            encoder->opus_cbr = opus_attr->cbr;
            encoder->opus_cvbr = opus_attr->cvbr;
            encoder->opus_complexity = opus_attr->complexity;
            encoder->opus_mode = opus_attr->mode;
            encoder->opus_application = OPUS_APPLICATION_AUDIO;
        }
        break;

    case AUDIO_FORMAT_TYPE_SBC:
        {
            T_AUDIO_SBC_ATTR *sbc_attr;
            sbc_attr = &(format_info->attr.sbc);

            encoder->algorithm = ALGORITHM_SBC;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = (sbc_attr->chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO ? 0 : 1);
            encoder->sample_rate = sbc_attr->sample_rate;
            encoder->bit_res = 0;
            encoder->samples_per_frame = sbc_attr->block_length * sbc_attr->subband_num *
                                         (sbc_attr->chann_mode == AUDIO_SBC_CHANNEL_MODE_MONO ? 1 : 2);

            sbc_param->sample_rate = sbc_attr->sample_rate;
            sbc_param->chann_mode = sbc_attr->chann_mode;
            sbc_param->block_length = sbc_attr->block_length;
            sbc_param->subband_num = sbc_attr->subband_num;
            sbc_param->allocation_method = sbc_attr->allocation_method;
            sbc_param->bitpool = sbc_attr->bitpool;
            sbc_param->frame_num = 1;
        }
        break;

    case AUDIO_FORMAT_TYPE_MSBC:
        {
            encoder->algorithm = ALGORITHM_MSBC;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
            encoder->sample_rate = 16000;
            encoder->bit_res = 0;
            encoder->samples_per_frame = MSBC_FRAME_SIZE;
        }
        break;

    case AUDIO_FORMAT_TYPE_PCM:
        {
            T_AUDIO_PCM_ATTR *pcm_attr;
            pcm_attr = &(format_info->attr.pcm);

            encoder->algorithm = ALGORITHM_USB_AUDIO;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = (pcm_attr->chann_num == 1 ? STREAM_CHANNEL_OUTPUT_MONO :
                                   STREAM_CHANNEL_OUTPUT_STEREO);
            encoder->sample_rate = pcm_attr->sample_rate;
            encoder->samples_per_frame = pcm_attr->frame_length / pcm_attr->chann_num / pcm_attr->bit_width * 8;
            switch (pcm_attr->bit_width)
            {
            case 16:
                {
                    encoder->bit_res = 0;
                }
                break;

            case 24:
                {
                    encoder->bit_res = 1;
                }
                break;

            default:
                {
                    encoder->bit_res = 0;
                }
                break;
            }
        }
        break;

    case AUDIO_FORMAT_TYPE_LC3:
        {
            T_AUDIO_LC3_ATTR *lc3_attr;

            lc3_attr = &(format_info->attr.lc3);

            encoder->comm_header.encode_algorithm = ALGORITHM_LC3;
            encoder->comm_header.sample_rate =  lc3_attr->sample_rate;
            encoder->comm_header.frame_size_per_ch = audio_codec_frame_size_get(format_type, lc3_attr);
            encoder->comm_header.channel_num_per_block = audio_codec_chann_num_get(format_type, lc3_attr);
            encoder->comm_header.bit_width = 0;

            encoder->encoder.frame_duration = lc3_attr->frame_duration;
            encoder->encoder.blocks_per_sdu = 1;
            encoder->encoder.channel_allocation = lc3_attr->chann_location;
            encoder->encoder.octets_per_frame = lc3_attr->frame_length;
            encoder->encoder.max_transport_latency_ms = 50;
            encoder->encoder.sdu_interval = audio_codec_frame_duration_get(format_type, lc3_attr);
            encoder->encoder.presentation_delay_us = lc3_attr->presentation_delay;
            encoder->encoder.retransmission_num = 0;
            encoder->encoder.framed_unframed = 0;
            encoder->encoder.bis_cis = 0;
            encoder->encoder.encode_packet_format = 0;

            encoder->algorithm = ALGORITHM_LC3;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = audio_codec_chann_num_get(format_type, lc3_attr) - 1;
            encoder->sample_rate = lc3_attr->sample_rate;
            encoder->bit_res = 0;
            encoder->samples_per_frame = audio_codec_frame_size_get(format_type, lc3_attr);
        }
        break;

    case AUDIO_FORMAT_TYPE_CVSD:
        {
            T_AUDIO_CVSD_ATTR *cvsd_attr;

            cvsd_attr = &(format_info->attr.cvsd);

            encoder->algorithm = ALGORITHM_CVSD;
            encoder->sub_type = AAC_TYPE_LATM_NORMAL;
            encoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
            encoder->sample_rate = cvsd_attr->sample_rate;
            encoder->bit_res = 0;
            encoder->samples_per_frame = audio_codec_frame_size_get(format_type, cvsd_attr);
        }
        break;
    }

    return true;
}

static bool analog_encode_decode_set(T_DSP_MGR_SESSION *session, T_AUDIO_FORMAT_INFO *format_info)
{
    T_DSP_IPC_ENCODER *encoder;
    T_DSP_IPC_DECODER *decoder;

    session->decoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_DECODER));

    if (session->decoder_param == NULL)
    {
        return false;
    }

    session->encoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_ENCODER));

    if (session->encoder_param == NULL)
    {
        os_mem_free(session->decoder_param);
        return false;
    }

    decoder = session->decoder_param;
    encoder = session->encoder_param;

    decoder->algorithm = ALGORITHM_LINE_IN;
    decoder->sub_type = AAC_TYPE_LATM_NORMAL;
    decoder->chann_mode = STREAM_CHANNEL_OUTPUT_STEREO;
    decoder->sample_rate = 48000;
    decoder->bit_res = 0;
    decoder->samples_per_frame = 0;

    encoder->algorithm = ALGORITHM_LINE_IN;
    encoder->sub_type = AAC_TYPE_LATM_NORMAL;
    encoder->chann_mode = STREAM_CHANNEL_OUTPUT_STEREO;
    encoder->sample_rate = 48000;
    encoder->bit_res = 0;
    encoder->samples_per_frame = 0;

    return true;
}

static bool notification_decode_set(T_DSP_MGR_SESSION *session, T_AUDIO_FORMAT_INFO *format_info)
{
    T_DSP_IPC_DECODER *decoder;

    session->decoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_DECODER));

    if (session->decoder_param == NULL)
    {
        return false;
    }

    decoder = session->decoder_param;

    decoder->bit_res = 0;
    decoder->algorithm = ALGORITHM_PURE_STREAM;
    decoder->chann_mode = STREAM_CHANNEL_OUTPUT_MONO;
    decoder->sub_type = AAC_TYPE_LATM_NORMAL;
    decoder->sample_rate = 16000;
    decoder->samples_per_frame = MSBC_FRAME_SIZE;

    return true;
}

static bool apt_encode_decode_set(T_DSP_MGR_SESSION *session, T_AUDIO_FORMAT_INFO *format_info)
{
    T_DSP_IPC_ENCODER *encoder;
    T_DSP_IPC_DECODER *decoder;

    session->decoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_DECODER));

    if (session->decoder_param == NULL)
    {
        return false;
    }

    session->encoder_param = os_mem_zalloc2(sizeof(T_DSP_IPC_ENCODER));

    if (session->encoder_param == NULL)
    {
        os_mem_free(session->decoder_param);
        return false;
    }

    decoder = session->decoder_param;
    encoder = session->encoder_param;

    decoder->algorithm = ALGORITHM_LINE_IN;
    decoder->sub_type = AAC_TYPE_LATM_NORMAL;
    decoder->chann_mode = STREAM_CHANNEL_OUTPUT_STEREO;
    decoder->sample_rate = 48000;
    decoder->bit_res = 0;
    decoder->samples_per_frame = 0;

    encoder->algorithm = ALGORITHM_LINE_IN;
    encoder->sub_type = AAC_TYPE_LATM_NORMAL;
    encoder->chann_mode = STREAM_CHANNEL_OUTPUT_STEREO;
    encoder->sample_rate = 48000;
    encoder->bit_res = 0;
    encoder->samples_per_frame = 0;

    return true;
}

T_DSP_MGR_SESSION_HANDLE dsp_mgr_session_create(T_DSP_SESSION_TYPE type, T_DSP_SESSION_CFG config)
{
    int8_t ret = 0;
    T_DSP_MGR_SESSION *session;

    session = os_mem_zalloc2(sizeof(T_DSP_MGR_SESSION));
    if (session == NULL)
    {
        ret = 1;
        goto fail_alloc_item;
    }

    session->context               = config.context;
    session->callback              = config.callback;
    session->type                  = type;
    session->sport_handle          = config.sport_handle;
    session->src_transport_address = 0;
    session->snk_transport_address = 0;
    session->state                 = DSP_SESSION_STATE_IDLE;

    if (config.data_mode == AUDIO_STREAM_MODE_LOW_LATENCY)
    {
        session->data_mode = DSP_DATA_LOW_LATENCY_MODE;
    }
    else if (config.data_mode == AUDIO_STREAM_MODE_ULTRA_LOW_LATENCY)
    {
        session->data_mode = DSP_DATA_ULTRA_LOW_LATENCY_MODE;
    }
    else if (config.data_mode == AUDIO_STREAM_MODE_DIRECT)
    {
        session->data_mode = DSP_DATA_DIRECT_MODE;
    }
    else if (config.data_mode == AUDIO_STREAM_MODE_HIGH_STABILITY)
    {
        session->data_mode = DSP_DATA_HIGH_STABILITY;
    }
    else
    {
        session->data_mode = DSP_DATA_NORMAL_MODE;
    }

    switch (type)
    {
    case DSP_SESSION_TYPE_AUDIO:
        {
            if (!audio_decode_set(session, config.format_info))
            {
                ret = 2;
                goto fail_alloc_item;
            }
        }
        break;

    case DSP_SESSION_TYPE_VOICE:
        {

            if (!voice_encode_decode_set(session, config.format_info))
            {
                ret = 3;
                goto fail_alloc_item;
            }
        }
        break;

    case DSP_SESSION_TYPE_RECORD:
        {
            if (!record_encode_set(session, config.format_info))
            {
                ret = 4;
                goto fail_alloc_item;
            }
        }
        break;

    case DSP_SESSION_TYPE_ANALOG:
        {
            if (!analog_encode_decode_set(session, config.format_info))
            {
                ret = 5;
                goto fail_alloc_item;
            }
        }
        break;

    case DSP_SESSION_TYPE_TONE:
    case DSP_SESSION_TYPE_VP:
        {

            if (!notification_decode_set(session, config.format_info))
            {
                ret = 6;
                goto fail_alloc_item;
            }
        }
        break;

    case DSP_SESSION_TYPE_APT:
        {
            if (!apt_encode_decode_set(session, config.format_info))
            {
                ret = 7;
                goto fail_alloc_item;
            }
        }
        break;
    }

    os_queue_in(&(dsp_mgr_db.session_queue), session);

    return (T_DSP_MGR_SESSION_HANDLE)session;

fail_alloc_item:
    DIPC_PRINT_ERROR1("dsp_mgr_session_create: failed , ret = %d", -ret);
    return NULL;
}

bool dsp_mgr_session_destroy(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session = (T_DSP_MGR_SESSION *)handle;

    if (session->decoder_param != NULL)
    {
        os_mem_free(session->decoder_param);
    }

    if (session->encoder_param != NULL)
    {
        os_mem_free(session->encoder_param);
    }

    if (session->sbc_param != NULL)
    {
        os_mem_free(session->sbc_param);
    }

    os_queue_delete(&(dsp_mgr_db.session_queue), session);
    os_mem_free(session);

    return true;
}

RAM_TEXT_SECTION bool dsp_lps_check(void)
{
    T_DSP_STATE dsp_state;

    dsp_state = dsp_mgr_dsp_state_get();

    if (dsp_state == DSP_STATE_OFF)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool dsp_mgr_power_on_check(void)
{
    return (dsp_hal_boot_ref_cnt_get() != 0) ? true : false;
}

static void notification_wait_finish(void *handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    if (session)
    {
        if (session->callback != NULL)
        {
            session->callback(session->context, DSP_MGR_EVT_NOTIFICATION_FINISH, 0);
        }
    }

    return ;
}

static bool dsp_mgr_decoder_set(T_DSP_MGR_SESSION *owner)
{
    T_DECODE_CONTEXT *context;

    context = &dsp_mgr_db.decoder;

    if ((context->busy == false) && (owner->decoder_param != NULL))
    {
        context->owner = owner;
    }

    DIPC_PRINT_TRACE2("dsp_mgr_decoder_set: owner %p, busy %d", owner, context->busy);

    return true;
}

static bool dsp_mgr_encoder_set(T_DSP_MGR_SESSION *owner)
{
    T_ENCODE_CONTEXT *context;

    context = &dsp_mgr_db.encoder;

    if ((context->busy == false) && (owner->encoder_param != NULL))
    {
        context->owner = owner;
    }

    DIPC_PRINT_TRACE2("dsp_mgr_encoder_set: owner %p, busy %d", owner, context->busy);

    return true;
}

bool dsp_mgr_session_decoder_effect_control(T_DSP_MGR_SESSION_HANDLE handle,
                                            uint8_t                  action)
{
    T_DSP_MGR_SESSION *session;
    bool ret = false;

    session = (T_DSP_MGR_SESSION *)handle;

    if (os_queue_search(&dsp_mgr_db.session_queue, session))
    {
        ret = dipc_decoder_effect_control(session->type, action);
    }

    return ret;
}

bool dsp_mgr_session_encoder_effect_control(T_DSP_MGR_SESSION_HANDLE handle,
                                            uint8_t                  action)
{
    T_DSP_MGR_SESSION *session;
    bool ret = false;

    session = (T_DSP_MGR_SESSION *)handle;

    if (os_queue_search(&dsp_mgr_db.session_queue, session))
    {
        ret = dipc_encoder_effect_control(session->type, action);
    }

    return ret;
}

bool dsp_mgr_session_enable(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;
    T_DSP_MGR_SESSION *session;
    T_DSP_STATE dsp_state;
    int ret;

    ret = 0;
    session = (T_DSP_MGR_SESSION *)handle;

    if (session == NULL)
    {
        ret = 1;
        goto enable_fail;
    }

    if (session->context == NULL)
    {
        ret = 2;
        goto enable_fail;
    }

    dsp_mgr_decoder_set(session);
    dsp_mgr_encoder_set(session);

    dsp_state = dsp_mgr_dsp_state_get();
    event_group = dsp_mgr_event_group_get();

    session->state = DSP_SESSION_STATE_STARTING;

    if (dsp_state != DSP_STATE_FW_READY)
    {
        dsp_state_machine(DSP_EVENT_START_FW, (uint32_t)session);
    }
    else
    {
        dsp_send_msg(DSP_MSG_PREPARE_READY, 0, session, 0);
    }

    if ((session->type == DSP_SESSION_TYPE_TONE) ||
        (session->type == DSP_SESSION_TYPE_VP))
    {
        if (!sys_event_flag_check(event_group,
                                  DSP_MGR_EVENT_FLAG_WAIT_NOTIFICATION_FINISH))
        {
            sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_SET,
                                DSP_MGR_EVENT_FLAG_WAIT_NOTIFICATION_FINISH);

            sys_event_flag_wait(event_group,
                                notification_wait_finish,
                                session,
                                DSP_MGR_EVENT_FLAG_WAIT_NOTIFICATION_FINISH,
                                SYS_EVENT_FLAG_TYPE_CLEAR_AND);
        }
    }

    return true;

enable_fail:
    DIPC_PRINT_TRACE1("dsp_mgr_session_enable: fail ret:%d", -ret);
    return false;
}

static void dsp_apt_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;
    T_DSP_SUBSTATE sport0_ad_state;
    T_DSP_SUBSTATE sport0_da_state;

    session = (T_DSP_MGR_SESSION *)handle;
    sport0_ad_state = dsp_mgr_sub_state_get(DIPC_GATE_ID0, DIPC_DIRECTION_RX);
    sport0_da_state = dsp_mgr_sub_state_get(DIPC_GATE_ID0, DIPC_DIRECTION_TX);

    if ((sport0_da_state == DSP_SUBSTATE_IDLE) ||
        (sport0_ad_state == DSP_SUBSTATE_IDLE))
    {
        if (sport0_da_state == DSP_SUBSTATE_RUNNING)
        {
            dsp_ipc_set_encoder(*(session->encoder_param), false);
        }
        else if (sport0_ad_state == DSP_SUBSTATE_RUNNING)
        {
            dsp_ipc_set_decoder(*(session->decoder_param), false);
        }
        else
        {
            dsp_ipc_set_decoder(*(session->decoder_param), false);
            dsp_ipc_set_encoder(*(session->encoder_param), false);
        }

        dsp_ipc_set_stream_channel_out_config(dsp_mgr_db.sport0_da_chann, false);
    }
}

static void dsp_record_cfg_setting(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    dsp_ipc_set_data_mode(session->data_mode, session->type);
    if (session->encoder_param->algorithm == ALGORITHM_LC3)
    {
        dsp_ipc_set_lc3_encoder(session->encoder_param->comm_header, session->encoder_param->encoder);
    }
    else
    {
        dsp_ipc_set_encoder(*(session->encoder_param), false);
    }

    if (session->encoder_param->algorithm == ALGORITHM_SBC)
    {
        dsp_ipc_set_sbc_encoder_hdr_config(*(session->sbc_param));
    }

    dsp_ipc_set_stream_channel_in_config(dsp_mgr_db.sport0_ad_chann);
}

#if 0
static void dsp_write_vad_register(T_VAD_SCENARIO vad_scenario)
{
    uint32_t scenario_num;
    uint32_t each_scenario_len;
    uint32_t *vad_register_config;
    uint32_t register_value;
    uint32_t offset_addr;

    DIPC_PRINT_TRACE1("dsp_write_vad_register: vad_scenario %d", vad_scenario);

    each_scenario_len = (dsp_mgr_db.vad_param.header)->each_scenario_len;
    scenario_num = (dsp_mgr_db.vad_param.header)->scenario_num;

    if (vad_scenario > scenario_num)
    {
        DIPC_PRINT_ERROR0("dsp_write_vad_register: vad_scenario error");
        return;
    }

    vad_register_config = (uint32_t *)(dsp_mgr_db.vad_param.param + vad_scenario * each_scenario_len);

    for (uint32_t i = 0; i < each_scenario_len / 4; i++)
    {
        offset_addr = i * 4;
        register_value = *(vad_register_config + i);
        HAL_WRITE32(VAD_BASE_ADDR, offset_addr, register_value);
    }
}
#endif

void dsp_vad_param_set(T_VAD_SCENARIO vad_scenario_state)
{
#if 0
    dsp_write_vad_register(vad_scenario_state);

    dsp_ipc_set_voice_trigger((uint32_t)!sys_cfg_const.audio_secondary_spk_enable);
#endif
}

static void dsp_mgr_sport_set_post_cback(T_DSP_MGR_SESSION *session, uint32_t wait_flag)
{
    return ;
}

static uint32_t dsp_mgr_sport_set(uint8_t gate_id, uint8_t direction, T_DSP_MGR_SESSION *session)
{
    T_DSP_IPC_SPORT_CFG config;
    T_AUDIO_ROUTE_SPORT_CFG rtx_cfg;
    uint8_t ch_mode;

    if (direction & BIT(DIPC_DIRECTION_TX))
    {
        memset(&config, 0, sizeof(T_DSP_IPC_SPORT_CFG));
        rtx_cfg = audio_route_sport_cfg_get((T_AUDIO_ROUTE_SPORT_IDX)gate_id,
                                            AUDIO_ROUTE_SPORT_CH_DIR_TX);
        ch_mode = (sport_mgr_tx_channel_mode_get(session->sport_handle) == 0) ?
                  DIPC_CH_MODE_STEREO : DIPC_CH_MODE_MONO;

        config.sport_id = gate_id;
        config.rtx = 1;
        config.channel_count = (rtx_cfg.mode + 1) * 2;
        config.data_length = sport_data_len_table[rtx_cfg.data_len];
        config.role = rtx_cfg.role;
        config.channel_mode = ch_mode;
        config.bridge = rtx_cfg.bridge;
        config.sample_rate = sport_mgr_sample_rate_get((uint32_t)gate_id, SPORT_DIRECTION_DA);

        dsp_ipc_sport_set(&config);
    }

    if (direction & BIT(DIPC_DIRECTION_RX))
    {
        memset(&config, 0, sizeof(T_DSP_IPC_SPORT_CFG));
        rtx_cfg = audio_route_sport_cfg_get((T_AUDIO_ROUTE_SPORT_IDX)gate_id,
                                            AUDIO_ROUTE_SPORT_CH_DIR_RX);
        ch_mode = (sport_mgr_rx_channel_mode_get(session->sport_handle) == 0) ?
                  DIPC_CH_MODE_STEREO : DIPC_CH_MODE_MONO;

        config.sport_id = gate_id;
        config.rtx = 0;
        config.channel_count = (rtx_cfg.mode + 1) * 2;
        config.data_length = sport_data_len_table[rtx_cfg.data_len];
        config.role = rtx_cfg.role;
        config.channel_mode = ch_mode;
        config.bridge = rtx_cfg.bridge;
        config.sample_rate = sport_mgr_sample_rate_get((uint32_t)gate_id, SPORT_DIRECTION_AD);

        dsp_ipc_sport_set(&config);
    }

    return 0;
}

bool dsp_mgr_session_disable(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;
    int32_t ret = 0;

    session = (T_DSP_MGR_SESSION *)handle;
    if (session == NULL)
    {
        ret = 1;
        goto fail_invalid_handle;
    }

    if (session->context == NULL)
    {
        ret = 2;
        goto fail_invalid_context;
    }

    session->state = DSP_SESSION_STATE_STOPPING;

    dsp_mgr_deployment_traverse(session, dsp_mgr_sport_stop, dsp_mgr_gate_stop_post_cback);

    return true;

fail_invalid_context:
fail_invalid_handle:
    DIPC_PRINT_ERROR1("dsp_mgr_session_disable: failed %d", -ret);
    return false;
}

bool dsp_mgr_power_on(void)
{
    dsp_state_machine(DSP_EVENT_POWER_ON_DSP, 0);

    return true;
}

bool dsp_mgr_power_off(void)
{
    dsp_state_machine(DSP_EVENT_POWER_DOWN_DSP, 0);

    return true;
}

void dsp_mgr_load_finish(void)
{
    dsp_state_machine(DSP_EVENT_IMAGE_LOAD_FINISH, 0);
}

bool dsp_mgr_session_run(T_DSP_MGR_SESSION_HANDLE handle)
{
    T_DSP_MGR_SESSION *session;

    session = (T_DSP_MGR_SESSION *)handle;

    cfg_cback[session->type](handle);

    dsp_mgr_deployment_traverse(session, dsp_mgr_sport_set, dsp_mgr_sport_set_post_cback);

    if (session->type == DSP_SESSION_TYPE_RECORD)
    {
        dsp_mgr_encoder_action(AUDIO_CATEGORY_RECORD, DSP_IPC_ACTION_START);
    }

    dsp_mgr_deployment_traverse(session, dsp_mgr_sport_start, dsp_mgr_sport_start_post_cback);

    return true;
}

static void dsp_mgr_notification_stop(T_DSP_MGR_SESSION_HANDLE handle,
                                      P_NOTIFICATION_CBACK cback)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;

    event_group = dsp_mgr_event_group_get();

    if (sys_event_flag_check(event_group, DSP_MGR_EVENT_FLAG_NOTIFICATION_START))
    {
        cback();
    }
    else
    {
        dsp_send_msg(DSP_MSG_FAKE_ACTION, DSP_MSG_SUB_TYPE_NOTIFICATION_END, handle, 0);
    }

    return ;
}

bool dsp_mgr_voice_prompt_start(uint32_t cfg, uint32_t cfg_bt_clk_mix)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;

    event_group = dsp_mgr_event_group_get();

    if (!sys_event_flag_check(event_group, DSP_MGR_EVENT_FLAG_NOTIFICATION_START))
    {
        sys_event_flag_post(event_group,
                            SYS_EVENT_FLAG_OPT_SET,
                            DSP_MGR_EVENT_FLAG_NOTIFICATION_START);

        return dsp_ipc_voice_prompt_start(cfg, cfg_bt_clk_mix);
    }

    return false;
}


bool dsp_mgr_voice_prompt_send(uint8_t *p_data, uint32_t len)
{
    return dsp_ipc_voice_prompt_send(p_data, len);
}

void dsp_mgr_voice_prompt_stop(T_DSP_MGR_SESSION_HANDLE handle)
{
    dsp_mgr_notification_stop(handle, dsp_ipc_voice_prompt_end);

    return ;
}

bool dsp_mgr_composite_start(void *p_data, uint32_t len)
{
    T_SYS_EVENT_GROUP_HANDLE event_group;

    event_group = dsp_mgr_event_group_get();

    if (!sys_event_flag_check(event_group, DSP_MGR_EVENT_FLAG_NOTIFICATION_START))
    {
        sys_event_flag_post(event_group,
                            SYS_EVENT_FLAG_OPT_SET,
                            DSP_MGR_EVENT_FLAG_NOTIFICATION_START);

        return dsp_ipc_composite_data_send(p_data, len);
    }

    return false;
}

void dsp_mgr_composite_stop(T_DSP_MGR_SESSION_HANDLE handle)
{
    dsp_mgr_notification_stop(handle, dsp_ipc_set_action_control);

    return ;
}

uint8_t dsp_mgr_dsp2_ref_get(void)
{
    return dsp_mgr_db.dsp2_ref;
}

void dsp_mgr_dsp2_ref_increment(void)
{
    dsp_mgr_db.dsp2_ref++;
}

void dsp_mgr_dsp2_ref_decrement(void)
{
    dsp_mgr_db.dsp2_ref--;
}

void dsp_mgr_signal_proc_start(T_DSP_MGR_SESSION_HANDLE handle, uint32_t timestamp,
                               uint8_t clk_ref, bool sync_flag)
{
    T_AUDIO_ROUTE_PHYSICAL_PATH_GROUP physical_path_group;
    T_AUDIO_ROUTE_PHYSICAL_PATH *physical_path;
    T_SYS_EVENT_GROUP_HANDLE event_group;
    T_DSP_MGR_SESSION *session;
    uint32_t event_flag;
    uint8_t i;

    event_flag = 0;
    session = (T_DSP_MGR_SESSION *)handle;
    event_group = dsp_mgr_event_group_get();

    physical_path_group = audio_route_physical_path_take((T_AUDIO_CATEGORY)session->type);

    if (physical_path_group.physical_path_num == 0)
    {
        goto func_end;
    }

    physical_path = physical_path_group.physical_path;

    for (i = 0; i < physical_path_group.physical_path_num; ++i)
    {
        if (physical_path[i].sport_idx < DIPC_GATE_MAX)
        {
            if (physical_path[i].sport_ch_dir == AUDIO_ROUTE_SPORT_CH_DIR_TX)
            {
                event_flag = gate_sig_proc_flag[physical_path[i].sport_idx];

                if (!sys_event_flag_check(event_group, event_flag))
                {
                    dsp_remote_init(handle, clk_ref, timestamp, sync_flag);
                    dsp_ipc_set_signal_proc_start(timestamp, physical_path[i].sport_idx);
                    sys_event_flag_post(event_group, SYS_EVENT_FLAG_OPT_SET, event_flag);
                }
            }
        }
    }

func_end:

    audio_route_physical_path_give(&physical_path_group);

    return ;

}
