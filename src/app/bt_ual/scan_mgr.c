/*
 *  Copyright (C) 2021 Realtek Semiconductor Corporation.
 *
 */

#include <stdint.h>
#include <string.h>
#include <os_mem.h>
#include <trace.h>
#include "ual_types.h"
#include "ual_list.h"
#include "ual_upperstack_cfg.h"

#include "btm.h"
#include "gap_msg.h"
#include "gap_ext_scan.h"
#include "scan_mgr.h"
#include "dev_mgr.h"
#include "ual_api_types.h"
#include "ble_privacy.h"
#include "ual_bluetooth.h"


#define LE_ADV_DATA_LEN_MAX              (0x1F)
#define LE_RSP_DATA_LEN_MAX              (0x1F)
#define LE_EA_RSP_DATA_LEN_MAX           (0xFB)
#define LE_EA_ADV_DATA_LEN_MAX           (0x0672)
typedef struct
{
    uint8_t       scan_mode;
    bool          scan_enabled;
    T_SCAN_STATE    scan_state;
} T_SCAN_MGR_CB;

typedef enum
{
    START_SCAN_REQ_EVT,
    STOP_SCAN_REQ_EVT,
    SCAN_STATE_CHANGE_EVT,
    SUSPEND_SCAN_REQ_EVT,
    RESUME_SCAN_REQ_EVT,
    CAP_MAX_EVT
} SCAN_SM_EVENT;

/* state machine action enumeration list */
enum
{
    HDL_START_LE_SCAN_ACT,
    HDL_STOP_LE_SCAN_ACT,
    HDL_SCAN_STATE_CHG_ACT,
    HDL_SUSPEND_REQ_ACT,
    HDL_RESUME_REQ_ACT,
    SCAN_NUM_ACTIONS
};

#define SCAN_ACT_IGNORE SCAN_NUM_ACTIONS


typedef struct
{
    uint8_t scan_mode;
    uint8_t filter_policy;
} T_SCAN_PARAMS;
/* union of all event data types */
typedef struct
{
    union
    {
        T_SCAN_PARAMS                      scan_params;
        T_LE_EXT_SCAN_STATE_CHANGE_INFO   state_change_info;
    } data;
} T_SCAN_ACTION;


/* type for action functions */
typedef bool (*T_SCAN_ACTION_FUNCS)(T_SCAN_ACTION *p_data);

/* state table for idle state */
const uint8_t scan_state_idle[][2] =
{
    /* Event                            Action                    Next state */
    /* START_SCAN_REQ_EVT      */   {HDL_START_LE_SCAN_ACT,   SCAN_ENABLE_STATE},
    /* STOP_SCAN_REQ_EVT  */        {SCAN_ACT_IGNORE,         SCAN_IDLE_STATE},
    /* SCAN_STATE_CHANGE_EVT  */    {HDL_SCAN_STATE_CHG_ACT,  SCAN_IDLE_STATE},
    /* SUSPEND_SCAN_REQ_EVT  */     {HDL_SUSPEND_REQ_ACT,     SCAN_SUSPEND_STATE},
    /* RESUME_SCAN_REQ_EVT  */      {SCAN_ACT_IGNORE,         SCAN_IDLE_STATE},
};

/* state table for suspend state */
const uint8_t scan_state_suspend[][2] =
{
    /* Event                            Action                    Next state */
    /* START_SCAN_REQ_EVT      */   {HDL_START_LE_SCAN_ACT,   SCAN_SUSPEND_STATE},
    /* STOP_SCAN_REQ_EVT  */        {HDL_STOP_LE_SCAN_ACT,    SCAN_SUSPEND_STATE},
    /* SCAN_STATE_CHANGE_EVT  */    {HDL_SCAN_STATE_CHG_ACT,  SCAN_SUSPEND_STATE},
    /* SUSPEND_SCAN_REQ_EVT  */     {SCAN_ACT_IGNORE,         SCAN_SUSPEND_STATE},
    /* RESUME_SCAN_REQ_EVT  */      {HDL_RESUME_REQ_ACT,      SCAN_IDLE_STATE},
};

/* state table for enable state */
const uint8_t scan_state_enable[][2] =
{
    /* Event                            Action                    Next state */
    /* START_SCAN_REQ_EVT      */   {SCAN_ACT_IGNORE,         SCAN_ENABLE_STATE},
    /* STOP_SCAN_REQ_EVT  */        {HDL_STOP_LE_SCAN_ACT,    SCAN_IDLE_STATE},
    /* SCAN_STATE_CHANGE_EVT  */    {HDL_SCAN_STATE_CHG_ACT,  SCAN_ENABLE_STATE},
    /* SUSPEND_SCAN_REQ_EVT  */     {HDL_SUSPEND_REQ_ACT,     SCAN_SUSPEND_STATE},
    /* RESUME_SCAN_REQ_EVT  */      {HDL_RESUME_REQ_ACT,      SCAN_ENABLE_STATE},
};

/* type for state table */
typedef const uint8_t (*T_SCAN_STATE_TBL)[2];

/* state table */
const T_SCAN_STATE_TBL scan_state_tbl[] =
{
    scan_state_idle,
    scan_state_suspend,
    scan_state_enable,
};

static bool hdl_start_le_scan(T_SCAN_ACTION *p_data);
static bool hdl_stop_le_scan(T_SCAN_ACTION *p_data);
static bool hdl_scan_state_change(T_SCAN_ACTION *p_data);
static bool hdl_suspend_req(T_SCAN_ACTION *p_data);
static bool hdl_resume_req(T_SCAN_ACTION *p_data);


/* action functions */
const T_SCAN_ACTION_FUNCS scan_action_funcs[] =
{
    hdl_start_le_scan,
    hdl_stop_le_scan,
    hdl_scan_state_change,
    hdl_suspend_req,
    hdl_resume_req,
    NULL
};


static T_SCAN_MGR_CB scan_mgr;

static bool hdl_start_le_scan(T_SCAN_ACTION *p_data)
{
    T_GAP_LOCAL_ADDR_TYPE  local_bd_type = (T_GAP_LOCAL_ADDR_TYPE)ble_get_local_bd_type();
    T_GAP_SCAN_FILTER_POLICY  ext_scan_filter_policy = (T_GAP_SCAN_FILTER_POLICY)
                                                       p_data->data.scan_params.filter_policy;
    T_GAP_SCAN_FILTER_DUPLICATE  ext_scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_DISABLE;
    T_GAP_CAUSE cause;
    scan_mgr.scan_mode = p_data->data.scan_params.scan_mode;
    scan_mgr.scan_enabled = true;
    uint16_t ext_scan_duration = 0;
    uint16_t ext_scan_period = 0;
    uint8_t  scan_phys = GAP_EXT_SCAN_PHYS_1M_BIT;
    T_GAP_LE_EXT_SCAN_PARAM extended_scan_param[GAP_EXT_SCAN_MAX_PHYS_NUM];

    extended_scan_param[0].scan_type = (T_GAP_SCAN_MODE)p_data->data.scan_params.scan_mode;
    extended_scan_param[0].scan_interval = 200;
    extended_scan_param[0].scan_window = 190;
    extended_scan_param[1].scan_type = (T_GAP_SCAN_MODE)p_data->data.scan_params.scan_mode;
    extended_scan_param[1].scan_interval = 220;
    extended_scan_param[1].scan_window = 110;

    /* Initialize extended scan parameters */
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_LOCAL_ADDR_TYPE, sizeof(local_bd_type),
                          &local_bd_type);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PHYS, sizeof(scan_phys),
                          &scan_phys);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_DURATION, sizeof(ext_scan_duration),
                          &ext_scan_duration);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_PERIOD, sizeof(ext_scan_period),
                          &ext_scan_period);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_POLICY, sizeof(ext_scan_filter_policy),
                          &ext_scan_filter_policy);
    PROTOCOL_PRINT_INFO1("hdl_start_le_scan, filter %d", ext_scan_filter_duplicate);
    le_ext_scan_set_param(GAP_PARAM_EXT_SCAN_FILTER_DUPLICATES, sizeof(ext_scan_filter_duplicate),
                          &ext_scan_filter_duplicate);

    /* Initialize extended scan PHY parameters */
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_1M, &extended_scan_param[0]);
    le_ext_scan_set_phy_param(LE_SCAN_PHY_LE_CODED, &extended_scan_param[1]);

    if (scan_mgr.scan_state == SCAN_SUSPEND_STATE)
    {
        return true;
    }

    //le_ext_scan_report_filter(true, GAP_EXT_ADV_REPORT_BIT_CONNECTABLE_ADV, false);

    /* Enable extended scan */
    cause = le_ext_scan_start();
    if (cause != GAP_CAUSE_SUCCESS && cause != GAP_CAUSE_ALREADY_IN_REQ)
    {
        PROTOCOL_PRINT_ERROR0("hdl_start_le_scan fail");
        return true;
    }
    return true;
}

static bool hdl_stop_le_scan(T_SCAN_ACTION *p_data)
{
    scan_mgr.scan_mode = 0;
    scan_mgr.scan_enabled = false;
    PROTOCOL_PRINT_INFO0("hdl_stop_le_scan");
    le_ext_scan_stop();
    return true;
}

static bool hdl_scan_state_change(T_SCAN_ACTION *p_data)
{
    T_LE_EXT_SCAN_STATE_CHANGE_INFO *p_info = &p_data->data.state_change_info;
    T_GAP_CAUSE cause;
    T_BT_DISC_STATE state;

    APP_PRINT_INFO2("hdl_scan_state_change: state %x scan_state %x", p_info->state,
                    scan_mgr.scan_state);
    if (p_info->state == GAP_SCAN_STATE_IDLE)
    {
        if (scan_mgr.scan_state == SCAN_IDLE_STATE)
        {
            state = LE_SCAN_OFF;
            if (bt_hal_cbacks)
            {
                bt_hal_cbacks(UAL_ADP_LE_SCAN_STATE_CHANGE, (uint8_t *)&state, 1);
            }
        }
        else if (scan_mgr.scan_state == SCAN_ENABLE_STATE)
        {
            cause = le_ext_scan_start();
            if (cause != GAP_CAUSE_SUCCESS && cause != GAP_CAUSE_ALREADY_IN_REQ)
            {
                PROTOCOL_PRINT_ERROR0("restart scan fail");
            }
        }
        else if (scan_mgr.scan_state == SCAN_SUSPEND_STATE)
        {
#if BLE_PRIVACY_SPT
            handle_scan_stop_in_suspend();
#endif
        }
    }
    else if (p_info->state == GAP_SCAN_STATE_SCANNING)
    {
        if (scan_mgr.scan_state == SCAN_IDLE_STATE ||
            scan_mgr.scan_state == SCAN_SUSPEND_STATE)
        {
            le_ext_scan_stop();
        }
        else if (scan_mgr.scan_state == SCAN_ENABLE_STATE)
        {
            state = LE_SCAN_ON;
            if (bt_hal_cbacks)
            {
                bt_hal_cbacks(UAL_ADP_LE_SCAN_STATE_CHANGE, (uint8_t *)&state, 1);
            }
        }
    }

    return true;
}

static bool hdl_suspend_req(T_SCAN_ACTION *p_data)
{
    uint8_t state = ble_get_scan_st();
    if (state == GAP_SCAN_STATE_SCANNING)
    {
        le_ext_scan_stop();
    }
    return true;
}

static bool hdl_resume_req(T_SCAN_ACTION *p_data)
{
    uint8_t state = ble_get_scan_st();
    T_GAP_CAUSE cause;
    if (scan_mgr.scan_enabled)
    {
        cause = le_ext_scan_start();
        if (cause != GAP_CAUSE_SUCCESS && cause != GAP_CAUSE_ALREADY_IN_REQ)
        {
            PROTOCOL_PRINT_ERROR0("restart scan fail");
        }
        scan_mgr.scan_state = SCAN_ENABLE_STATE;    //change scan state right now
        return false;
    }

    return true;
}


static void scan_sm_execute(uint16_t event, T_SCAN_ACTION *p_data)
{
    T_SCAN_STATE_TBL state_table;
    uint8_t action;
    bool res = true;
    T_SCAN_STATE old_state = scan_mgr.scan_state;
    state_table = scan_state_tbl[scan_mgr.scan_state];
    event &= 0xff;
    PROTOCOL_PRINT_INFO1("scan_sm_execute: event %x", event);

    action = state_table[event][0];
    if (action != SCAN_ACT_IGNORE)
    {
        res = (*scan_action_funcs[action])(p_data);
    }

    if (res)
    {
        scan_mgr.scan_state = (T_SCAN_STATE)state_table[event][1];
    }

    PROTOCOL_PRINT_INFO3("scan_sm_execute State Change: [%d] -> [%d] after Event [0x%x]",
                         old_state, scan_mgr.scan_state, event);
}


static void ext_adv_clone(T_LE_ADV_INFO *info, T_LE_EXT_ADV_REPORT_INFO *report)
{
#define C(member) info->member = report->member
    C(event_type);
    C(data_status);
    info->bd_type = report->addr_type;
    memcpy(info->bd_addr, report->bd_addr, 6);
    C(primary_phy);
    C(secondary_phy);
    C(adv_sid);
    C(tx_power);
    C(rssi);
    C(peri_adv_interval);
    C(direct_addr_type);
    memcpy(info->direct_addr, report->direct_addr, 6);
    C(data_len);
    C(p_data);
}

static void ble_process_adv_addr(uint8_t **bd_addr, T_BLE_BD_TYPE *bd_type)
{
    /* map address to security record */
    bool match = ble_identity_addr_to_random_pseudo(bd_addr, bd_type, false);
    if (!match && BLE_IS_RESOLVE_BDA(*bd_addr))
    {
        T_BT_DEVICE *match_rec = ble_resolve_random_addr(*bd_addr);
        if (match_rec)
        {
            memcpy(match_rec->cur_rand_addr, *bd_addr, BD_ADDR_LEN);
            if (!ble_init_pseudo_addr(match_rec, *bd_addr, *bd_type))
            {
                // Assign the original address to be the current report address
                memcpy(*bd_addr, match_rec->pseudo_addr, BD_ADDR_LEN);
                *bd_type = match_rec->bd_type;
            }
        }
    }
}

static void ble_appearance_to_cod(uint16_t appearance, uint8_t *dev_class)
{
    dev_class[0] = 0;

    switch (appearance)
    {
    case GAP_GATT_APPEARANCE_GENERIC_PHONE:
        dev_class[1] = BTM_COD_MAJOR_PHONE;
        dev_class[2] = BTM_COD_MINOR_UNCLASSIFIED;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_COMPUTER:
        dev_class[1] = BTM_COD_MAJOR_COMPUTER;
        dev_class[2] = BTM_COD_MINOR_UNCLASSIFIED;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_REMOTE_CONTROL:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_REMOTE_CONTROL;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_THERMOMETER:
    case GAP_GATT_APPEARANCE_THERMOMETER_EAR:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_THERMOMETER;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_HEART_RATE_SENSOR:
    case GAP_GATT_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_HEART_PULSE_MONITOR;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_BLOOD_PRESSURE:
    case GAP_GATT_APPEARANCE_BLOOD_PRESSURE_ARM:
    case GAP_GATT_APPEARANCE_BLOOD_PRESSURE_WRIST:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_BLOOD_MONITOR;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_PULSE_OXIMETER:
    case GAP_GATT_APPEARANCE_FINGERTIP:
    case GAP_GATT_APPEARANCE_WRIST_WORN:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_PULSE_OXIMETER;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_GLUCOSE_METER:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_GLUCOSE_METER;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_WEIGHT_SCALE:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_WEIGHING_SCALE;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_RUNNING_WALKING_SENSOR:
    case GAP_GATT_APPEARANCE_RUNNING_WALKING_SENSOR_IN_SHOE:
    case GAP_GATT_APPEARANCE_RUNNING_WALKING_SENSOR_ON_SHOE:
    case GAP_GATT_APPEARANCE_RUNNING_WALKING_SENSOR_ON_HIP:
        dev_class[1] = BTM_COD_MAJOR_HEALTH;
        dev_class[2] = BTM_COD_MINOR_STEP_COUNTER;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_WATCH:
    case GAP_GATT_APPEARANCE_WATCH_SPORTS_WATCH:
        dev_class[1] = BTM_COD_MAJOR_WEARABLE;
        dev_class[2] = BTM_COD_MINOR_WRIST_WATCH;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_EYE_GLASSES:
        dev_class[1] = BTM_COD_MAJOR_WEARABLE;
        dev_class[2] = BTM_COD_MINOR_GLASSES;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_DISPLAY:
        dev_class[1] = BTM_COD_MAJOR_IMAGING;
        dev_class[2] = BTM_COD_MINOR_DISPLAY;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_MEDIA_PLAYER:
        dev_class[1] = BTM_COD_MAJOR_AUDIO;
        dev_class[2] = BTM_COD_MINOR_UNCLASSIFIED;
        break;
    case GAP_GATT_APPEARANCE_GENERIC_BARCODE_SCANNER:
    case GAP_GATT_APPEARANCE_BARCODE_SCANNER:
    case GAP_GATT_APPEARANCE_HUMAN_INTERFACE_DEVICE:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_UNCLASSIFIED;
        break;
    case GAP_GATT_APPEARANCE_KEYBOARD:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_KEYBOARD;
        break;
    case GAP_GATT_APPEARANCE_MOUSE:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_POINTING;
        break;
    case GAP_GATT_APPEARANCE_JOYSTICK:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_JOYSTICK;
        break;
    case GAP_GATT_APPEARANCE_GAMEPAD:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_GAMEPAD;
        break;
    case GAP_GATT_APPEARANCE_DIGITIZER_TABLET:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_DIGITIZING_TABLET;
        break;
    case GAP_GATT_APPEARANCE_CARD_READER:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_CARD_READER;
        break;
    case GAP_GATT_APPEARANCE_DIGITAL_PEN:
        dev_class[1] = BTM_COD_MAJOR_PERIPHERAL;
        dev_class[2] = BTM_COD_MINOR_DIGITAL_PAN;
        break;
    case GAP_GATT_APPEARANCE_UNKNOWN:
    case GAP_GATT_APPEARANCE_GENERIC_CLOCK:
    case GAP_GATT_APPEARANCE_GENERIC_TAG:
    case GAP_GATT_APPEARANCE_GENERIC_KEYRING:
    case GAP_GATT_APPEARANCE_GENERIC_CYCLING:
    case GAP_GATT_APPEARANCE_CYCLING_CYCLING_COMPUTER:
    case GAP_GATT_APPEARANCE_CYCLING_SPEED_SENSOR:
    case GAP_GATT_APPEARANCE_CYCLING_CADENCE_SENSOR:
    case GAP_GATT_APPEARANCE_CYCLING_POWER_SENSOR:
    case GAP_GATT_APPEARANCE_CYCLING_SPEED_AND_CADENCE_SENSOR:
    case GAP_GATT_APPEARANCE_GENERIC_OUTDOOR_SPORTS_ACTIVITY:
    case GAP_GATT_APPEARANCE_LOCATION_DISPLAY_DEVICE:
    case GAP_GATT_APPEARANCE_LOCATION_AND_NAVIGATION_DISPLAY_DEVICE:
    case GAP_GATT_APPEARANCE_LOCATION_POD:
    case GAP_GATT_APPEARANCE_LOCATION_AND_NAVIGATION_POD:
    default:
        dev_class[1] = BTM_COD_MAJOR_UNCLASSIFIED;
        dev_class[2] = BTM_COD_MINOR_UNCLASSIFIED;
    };
}

static void ble_update_scan_result(T_DISC_RESULT *p_result, uint8_t *p_data, uint16_t data_len)
{
    uint8_t pos = 0;
    uint8_t *pp;
    uint16_t length;
    uint8_t type;
    bool cod_updated = false;

    if (p_result == NULL || p_data == NULL)
    {
        return;
    }

    while (pos < data_len)
    {
        /* Length of the AD structure. */
        length = p_data[pos++];
        if (length < 1)
        {
            APP_PRINT_ERROR1("ble_update_scan_result: pos %d ", pos);
            break;
        }

        if (pos + length > data_len)
        {
            APP_PRINT_ERROR1("ble_update_scan_result: pos %d not enough data", pos);
            break;
        }
        /* Copy the AD Data to buffer. */
        pp = p_data + pos + 1;
        /* AD Type, one octet. */
        type = p_data[pos];
        switch (type)
        {
        case GAP_ADTYPE_FLAGS:
            {
            }
            break;
        case GAP_ADTYPE_APPEARANCE:
            {
                ble_appearance_to_cod((uint16_t)pp[0] | (pp[1] << 8), (uint8_t *)&p_result->cod);
                cod_updated = true;
            }
            break;
        case GAP_ADTYPE_16BIT_COMPLETE:
            {
                if (cod_updated)
                {
                    break;
                }
                for (uint8_t i = 0; i + 2 <= (length - 1); i = i + 2)
                {
                    if ((pp[i] | (pp[i + 1] << 8)) == UUID_SERVCLASS_LE_HID)
                    {
                        p_result->cod = (BTM_COD_MAJOR_PERIPHERAL << 8);
                        break;
                    }
                }
            }
            break;
        }
        pos += length;

    }
}

static bool ual_join_rsp_adv_report(T_DISC_RESULT *p_cur, T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    if ((!p_cur) || (!p_report))
    {
        return false;
    }
    uint8_t *p_temp = NULL;
    uint8_t max_len = LE_EA_RSP_DATA_LEN_MAX;

    if (p_report->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV)
    {
        max_len = LE_RSP_DATA_LEN_MAX;
    }

    if (p_report->data_len > max_len)
    {
        APP_PRINT_ERROR2("ual_join_rsp_adv_report: event %x len %d ", p_report->event_type,
                         p_report->data_len);
        if (p_cur->p_data)
        {
            os_mem_free(p_cur->p_data);
            p_cur->p_data = NULL;
            p_cur->data_len = 0;
        }
        return false;
    }

    if (p_report->data_len + p_cur->data_len == 0)
    {
        return true;
    }

    p_temp = os_mem_zalloc(RAM_TYPE_BT_UAL, p_report->data_len + p_cur->data_len);
    if (p_temp == NULL)
    {
        APP_PRINT_ERROR3("ual_join_rsp_adv_report: bd_addr %s status %x alloc fail",
                         TRACE_BDADDR(p_report->bd_addr), p_report->event_type, p_report->data_status);
        if (p_cur->p_data)
        {
            os_mem_free(p_cur->p_data);
            p_cur->p_data = NULL;
            p_cur->data_len = 0;
        }
        p_cur->adv_cmplt = false;
        return false;
    }

    if (p_cur->p_data)
    {
        if (p_cur->data_len)
        {
            memcpy(p_temp, p_cur->p_data, p_cur->data_len);
        }
        if (p_report->data_len)
        {
            memcpy(p_temp + p_cur->data_len, p_report->p_data, p_report->data_len);
        }
        os_mem_free(p_cur->p_data);
    }
    else
    {
        memcpy(p_temp, p_report->p_data, p_report->data_len);
    }
    p_cur->p_data = p_temp;
    p_cur->data_len += p_report->data_len;
    return true;

}

static bool ual_join_legacy_adv_report(T_DISC_RESULT *p_cur, T_LE_EXT_ADV_REPORT_INFO *p_report)
{
  static bool printf_flag = false;  
	
    if ((!p_cur) || (!p_report))
    {
        return false;
    }

    if (p_report->data_len > LE_ADV_DATA_LEN_MAX)
    {
        APP_PRINT_ERROR1("ual_join_legacy_adv_report: len %d ", p_report->data_len);
        if (p_cur->p_data)
        {
            os_mem_free(p_cur->p_data);
            p_cur->p_data = NULL;
            p_cur->data_len = 0;
        }
        return false;
    }

    if (p_cur->p_data)
    {
        os_mem_free(p_cur->p_data);
        p_cur->p_data = NULL;
        p_cur->data_len = 0;
    }

    if (p_report->data_len)
    {
        p_cur->p_data = os_mem_zalloc(RAM_TYPE_BT_UAL, p_report->data_len);
        if (p_cur->p_data == NULL)
        {
            APP_PRINT_ERROR2("ual_join_legacy_adv_report: %s event %x alloc fail",
                             TRACE_BDADDR(p_report->bd_addr), p_report->event_type);
            return false;
        }
        memcpy(p_cur->p_data, p_report->p_data, p_report->data_len);
        p_cur->data_len = p_report->data_len;
    }

    if (p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCANNABLE_ADV)
    {
      if(!printf_flag)
       {
         printf_flag = true;
         APP_PRINT_INFO1("ual_join_legacy_adv_report: %s wait rsp ", TRACE_BDADDR(p_report->bd_addr));
       }
        return false;
    }

    return true;
}

static bool ual_join_ext_adv_report(T_DISC_RESULT *p_cur, T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    if ((!p_cur) || (!p_report))
    {
        return false;
    }
    uint8_t *p_temp;

    if (p_cur->adv_cmplt)
    {
        if (p_cur->p_data)
        {
            os_mem_free(p_cur->p_data);
            p_cur->p_data = NULL;
            p_cur->data_len = 0;
        }
        p_cur->adv_cmplt = false;
    }

    if (p_report->data_len + p_cur->data_len > LE_EA_ADV_DATA_LEN_MAX)
    {
        APP_PRINT_ERROR2("ual_join_ext_adv_report: %s adv_len %d",
                         TRACE_BDADDR(p_report->bd_addr),
                         p_report->data_len + p_cur->data_len);
        if (p_cur->p_data)
        {
            os_mem_free(p_cur->p_data);
            p_cur->p_data = NULL;
            p_cur->data_len = 0;
        }
        return false;
    }

    if (p_report->data_status == GAP_EXT_ADV_EVT_DATA_STATUS_COMPLETE)
    {
        if (p_report->data_len + p_cur->data_len)
        {
            p_temp = os_mem_zalloc(RAM_TYPE_BT_UAL, p_report->data_len + p_cur->data_len);
            if (p_temp == NULL)
            {
                APP_PRINT_ERROR2("ual_join_ext_adv_report: bd_addr %s status %x alloc fail",
                                 TRACE_BDADDR(p_report->bd_addr), p_report->data_status);
                if (p_cur->p_data)
                {
                    os_mem_free(p_cur->p_data);
                    p_cur->p_data = NULL;
                    p_cur->data_len = 0;
                }
                return false;
            }
            if (p_cur->p_data)
            {
                memcpy(p_temp, p_cur->p_data, p_cur->data_len);
                memcpy(p_temp + p_cur->data_len, p_report->p_data, p_report->data_len);
                os_mem_free(p_cur->p_data);
            }
            else
            {
                memcpy(p_temp, p_report->p_data, p_report->data_len);
            }

            p_cur->p_data = p_temp;
            p_cur->data_len += p_report->data_len;
        }
        p_cur->adv_cmplt = true;
        if (p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCANNABLE_ADV)
        {
            APP_PRINT_INFO1("ual_join_ext_adv_report: %s wait rsp ", TRACE_BDADDR(p_report->bd_addr));
            return false;
        }
        return true;
    }
    else if (p_report->data_status == GAP_EXT_ADV_EVT_DATA_STATUS_MORE)
    {
        p_temp = os_mem_zalloc(RAM_TYPE_BT_UAL, p_report->data_len + p_cur->data_len);
        if (p_temp == NULL)
        {
            APP_PRINT_ERROR2("ual_join_ext_adv_report: bd_addr %s status %x alloc fail",
                             TRACE_BDADDR(p_report->bd_addr), p_report->data_status);
            if (p_cur->p_data)
            {
                os_mem_free(p_cur->p_data);
                p_cur->p_data = NULL;
                p_cur->data_len = 0;
            }
            return false;
        }

        if (p_cur->p_data)
        {
            memcpy(p_temp, p_cur->p_data, p_cur->data_len);
            memcpy(p_temp + p_cur->data_len, p_report->p_data, p_report->data_len);
            os_mem_free(p_cur->p_data);
        }
        else
        {
            memcpy(p_temp, p_report->p_data, p_report->data_len);
        }

        p_cur->p_data = p_temp;
        p_cur->data_len += p_report->data_len;
        return false;
    }
    else if (p_report->data_status == GAP_EXT_ADV_EVT_DATA_STATUS_TRUNCATED)
    {
        if (p_cur->p_data)
        {
            os_mem_free(p_cur->p_data);
            p_cur->p_data = NULL;
            p_cur->data_len = 0;
        }
        return false;
    }
    else
    {
        return false;
    }
}

void ual_handle_ext_adv_report(T_BT_ADAPTER *adapter,
                               T_LE_EXT_ADV_REPORT_INFO *p_report)
{
    struct bt_scan_client *client;
    T_UALIST_HEAD *pos, *n;
    T_LE_ADV_INFO adv_info;
    T_DISC_RESULT *p_cur;
    uint8_t bd_addr[6];
    T_BLE_BD_TYPE bd_type = BLE_ADDR_RANDOM;
    bool adv_complete = false;


    if (!adapter || !p_report)
    {
        return;
    }

    if (!adapter->le_scanning)
    {
        return;
    }

    /*APP_PRINT_TRACE6("connectable %d, scannable %d, direct %d, scan response %d, legacy %d, data status 0x%x",
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_CONNECTABLE_ADV,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCANNABLE_ADV,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_DIRECTED_ADV,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCAN_RESPONSE,
                    p_report->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV,
                    p_report->data_status);*/

    memcpy(bd_addr, p_report->bd_addr, 6);
    bd_type = p_report->addr_type;
    if (p_report->addr_type != GAP_REMOTE_ADDR_LE_ANONYMOUS)
    {
        uint8_t *pp = bd_addr;
        ble_process_adv_addr(&pp, &bd_type);
    }

    p_cur = ual_find_or_alloc_disc_db(bd_addr);
    if (p_cur == NULL)
    {
        APP_PRINT_ERROR1("ual_handle_ext_adv_report: bd_addr %s ual alloc fail", TRACE_BDADDR(bd_addr));
        return;
    }

    p_cur->bd_type = bd_type;

    if (p_report->event_type & GAP_EXT_ADV_REPORT_BIT_USE_LEGACY_ADV)
    {
        if (p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCAN_RESPONSE)
        {
            adv_complete = ual_join_rsp_adv_report(p_cur, p_report);
        }
        else
        {
            adv_complete = ual_join_legacy_adv_report(p_cur, p_report);
        }
    }
    else
    {
        if (p_report->event_type & GAP_EXT_ADV_REPORT_BIT_SCAN_RESPONSE)
        {
            adv_complete = ual_join_rsp_adv_report(p_cur, p_report);
        }
        else
        {
            adv_complete = ual_join_ext_adv_report(p_cur, p_report);
        }
    }

    if (!adv_complete)
    {
        return;
    }
    ext_adv_clone(&adv_info, p_report);

    adv_info.p_data = p_cur->p_data;
    adv_info.data_len = p_cur->data_len;
    ble_update_scan_result(p_cur, p_cur->p_data, p_cur->data_len);

    ualist_for_each_safe(pos, n, &adapter->scan_clients)
    {
        client = ualist_entry(pos, struct bt_scan_client, list);
        if (client->cback && (client->discovery_type & DISCOVERY_TYPE_LE))
        {
            client->cback(SCAN_RESULT_LE, (void *)&adv_info);
        }
    }
    if (p_cur->p_data != NULL)
    {
        os_mem_free(p_cur->p_data);
        p_cur->p_data = NULL;
        p_cur->data_len = 0;
        p_cur->adv_cmplt = false;
    }
}

bool bt_adap_start_le_scan(uint8_t scan_mode, uint8_t filter_policy)
{
    T_SCAN_ACTION data;
    data.data.scan_params.scan_mode = scan_mode;
    data.data.scan_params.filter_policy = filter_policy;
    scan_sm_execute(START_SCAN_REQ_EVT, &data);
    return true;
}


void scan_mgr_handle_scan_state_info(uint8_t state, uint16_t cause)
{
    T_SCAN_ACTION data;
    data.data.state_change_info.state = state;
    data.data.state_change_info.cause = cause;
    APP_PRINT_INFO2("scan_mgr_handle_scan_state_info: state %x cause %x", state, cause);

    scan_sm_execute(SCAN_STATE_CHANGE_EVT, &data);
}

bool bt_adap_stop_le_scan()
{
    scan_sm_execute(STOP_SCAN_REQ_EVT, NULL);
    return true;
}

bool le_scan_suspend_req(void)
{
    uint8_t state;
    scan_sm_execute(SUSPEND_SCAN_REQ_EVT, NULL);
    state = ble_get_scan_st();
    if (state == GAP_SCAN_STATE_IDLE &&
        scan_mgr.scan_state == SCAN_SUSPEND_STATE)
    {
        APP_PRINT_INFO0("le_scan_suspend_req, suspend success ");
        return true;
    }
    return false;
}

bool le_scan_resume_req(void)
{
    scan_sm_execute(RESUME_SCAN_REQ_EVT, NULL);
    return true;
}


T_SCAN_STATE le_scan_get_scan_state()
{
    return scan_mgr.scan_state;
}

void ble_scan_mgr_init()
{
    memset(&scan_mgr, 0, sizeof(T_SCAN_MGR_CB));
}

