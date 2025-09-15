#include <string.h>

#include "trace.h"
#include "os_mem.h"
#include "le_audio_service.h"
#include "codec_def.h"
#include "teams_call_control.h"
#include "app_usb_hid.h"
#include "app_timer.h"

#if LE_AUDIO_CCP_SERVER_SUPPORT

#define USB_HID_BUTTON_SUPPORT      0

#define CCP_CALL_LIST_MAX           (2)
#define CCP_INCOMING_TIMEOUT        (200)
#define CCP_TERMINATE_TIMEOUT       (200)
#define CCP_OUTGOING_TIMEOUT        (200)
#define CCP_TELEPHONY_MUTE_TIMEOUT  (200)
#define CCP_MICS_TIMEOUT            (100)

typedef enum
{
    CCP_TIMER_ID_INCOMING       = 0x01,       //telephony msg ring bit set true, new incoming call
    CCP_TIMER_ID_TERMINATE      = 0x02,       //telephony msg active bit set false, terminate active call
    CCP_TIMER_ID_OUTGOING       = 0x03,       //telephony msg active bit set true, new outgoing call
    CCP_TIMER_ID_MICS           = 0x04,
    CCP_TIMER_ID_TELEPHONY_MUTE = 0x05,
} T_CCP_TIMER_ID;

typedef struct
{
    uint16_t off_hook       : 1;    /* 0: terminate 1:accept */
    uint16_t speaker        : 1;
    uint16_t mute           : 1;    /* 0:un-mute 1:mute*/
    uint16_t ring           : 1;    /* 1: ring */
    uint16_t hold           : 1;    /* 1: hold*/
    uint16_t microphone     : 1;
    uint16_t on_line        : 1;
    uint16_t ringer         : 1;
    uint16_t rsv            : 8;
} T_TELEPHONY_HID_OUTPUT;

typedef struct
{
    uint8_t             call_id;
    bool                muted;
    T_TBS_CALL_STATE    call_state;
} T_CALL_DB;


typedef struct
{
    T_SERVER_ID         ccp_id;
    uint8_t             ring_bit;           //incoming call
    uint8_t             active_bit;         //outgoing call or accepted
    uint8_t             hold_bit;           //set active call (active_bit true call idx) to hold
    uint8_t             mute_bit;           //set active call (active_bit true call idx) to mute

    uint8_t             active_call_id;        //active state call
    uint8_t             ring_call_id;          //incoming call state
    uint8_t             hold_call_id;
    uint8_t             call_num;
    T_CALL_DB           call_list[CCP_CALL_LIST_MAX];

    bool                waiting_2_hold;
    uint8_t             pending_opcode;
    uint8_t             ccp_terminate_timer;
    uint8_t             ccp_incoming_timer;
    uint8_t             ccp_outgoing_timer;
    uint8_t             ccp_telephony_mute_timer;
    uint8_t             ccp_mics_timer;
    uint16_t            ccp_enabled_cccd;
    uint8_t             ccp_start_by_upstream;
} T_CALL_CONTROL_CB;



static T_CALL_CONTROL_CB ccp_db = {0x0};

static uint8_t ccp_timer_queue_id;

static uint8_t teams_alloc_outgoing_call(void);
static uint8_t teams_alloc_incoming_call(void);
static bool teams_terminate_call(uint8_t call_id);
static void teams_update_call_state(uint8_t call_id, T_TBS_CALL_STATE state);
extern bool unicast_src_update_ringtone_metadata(void);

static T_CALL_DB *ccp_get_call_handle_by_call_id(uint8_t call_id)
{
    T_CALL_DB *p_call = NULL;

    APP_PRINT_INFO1("ccp_get_call_handle_by_call_id %d", call_id);
    for (uint8_t i = 0; i < ccp_db.call_num; i++)
    {
        p_call = (T_CALL_DB *)&ccp_db.call_list[i];
        if (call_id == p_call->call_id)
        {
            return p_call;
        }
    }

    APP_PRINT_INFO0("ccp_get_call_handle_by_call_id NULL");
    return NULL;
}

static T_CALL_DB *ccp_alloc_call_handle(uint8_t call_id)
{
    T_CALL_DB *p_call = NULL;

    /*FIXME: check call_id is neccesary??*/
    for (uint8_t i = 0; i < CCP_CALL_LIST_MAX; i++)
    {
        p_call = (T_CALL_DB *)&ccp_db.call_list[i];
        if (p_call->call_id == 0)
        {
            p_call->call_id = call_id;
            p_call->call_state = TBS_CALL_STATE_RFU;
            return p_call;
        }
    }
    APP_PRINT_WARN1("ccp_alloc_call_handle id %x failed", call_id);
    return NULL;
}

static void ccp_free_call_handle(uint8_t call_id)
{
    T_CALL_DB *p_call = NULL;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return;
    }

    p_call->call_id = 0;
    p_call->call_state = TBS_CALL_STATE_RFU;
}

static void ccp_handle_incoming_timeout(void)
{
    uint8_t call_id = 0;
    T_CALL_DB *p_call = NULL;

    APP_PRINT_INFO1("incoming_timeout ring_bit %x", ccp_db.ring_bit);

    if (ccp_db.ring_bit)
    {
        call_id = teams_alloc_incoming_call();
        if (!call_id)
        {
            return;
        }

        p_call = ccp_alloc_call_handle(call_id);
        if (!p_call)
        {
            return;
        }


        ccp_db.ring_call_id = call_id;
        ccp_db.call_num++;
        APP_PRINT_INFO2("incoming_timeout ring_id %x num %x", call_id, ccp_db.call_num);
        teams_update_call_state(ccp_db.ring_call_id, TBS_CALL_STATE_INCOMING);
        unicast_src_update_ringtone_metadata();
    }
}

static void ccp_handle_terminate_timeout(void)
{
    APP_PRINT_INFO2("terminate_timeout active_call_id %x ring_call_id %x",
                    ccp_db.active_call_id,
                    ccp_db.ring_call_id);
    /* FIXME: what happen if call active, incoming call hanppen */
    if (ccp_db.ring_call_id)
    {
        if (teams_terminate_call(ccp_db.ring_call_id))
        {
            ccp_db.call_num--;
        }
        ccp_db.ring_call_id = 0;
    }
    else if (ccp_db.active_call_id)
    {
        if (teams_terminate_call(ccp_db.active_call_id))
        {
            ccp_db.call_num--;
        }
        ccp_db.active_call_id = 0;
        if (ccp_db.ccp_telephony_mute_timer)
        {
            app_stop_timer(&ccp_db.ccp_telephony_mute_timer);
#if DONGLE_LE_AUDIO
            extern void app_le_audio_set_mic_mute(uint8_t mic_mute);
            /*FIXME: MICS should send here*/
            app_le_audio_set_mic_mute(0);
#endif
        }
    }
}

static void ccp_handle_outgoing_timeout(void)
{
    uint8_t call_id = 0;
    T_CALL_DB *p_call = NULL;

    APP_PRINT_INFO1("outgoing_timeout active_bit %x", ccp_db.active_bit);
    if (ccp_db.active_bit)
    {
        call_id = teams_alloc_outgoing_call();
        if (!call_id)
        {
            return;
        }

        p_call = ccp_alloc_call_handle(call_id);
        if (!p_call)
        {
            return;
        }

        ccp_db.active_call_id = call_id;
        ccp_db.call_num++;
        APP_PRINT_INFO2("outgoing_timeout active_call_id %x num %x", call_id, ccp_db.call_num);
        teams_update_call_state(call_id, TBS_CALL_STATE_ACTIVE);
        unicast_src_update_ringtone_metadata();
    }
}

static void ccp_handle_mics_timeout(uint16_t param)
{
    T_TELEPHONY_HID_INPUT ctrl_code = {0x0};
    uint8_t mute = param & 0xFF;
    APP_PRINT_INFO2("ccp_handle_mics_timeout: mute_bit %x, new_mute %x", ccp_db.mute_bit, mute);
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
    if (ccp_db.mute_bit == mute)
    {
        return;
    }

    if (mute)
    {
        ctrl_code.mute = 0;
        ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
        app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
        ctrl_code.mute = 1;
        ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
        app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
    }
    else
    {
        ctrl_code.mute = 1;
        ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
        app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
        ctrl_code.mute = 0;
        ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
        app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
    }

#endif
}

static void ccp_handle_telephony_mute_timeout(uint16_t mute)
{
    if (mute != ccp_db.mute_bit)
    {
        return;
    }

    uint8_t call_id = 0;
    T_CALL_DB *p_call = NULL;

    if (ccp_db.active_call_id == 0)
    {
        return;
    }

    call_id = ccp_db.active_call_id;
    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return;
    }

    if (p_call->call_state != TBS_CALL_STATE_ACTIVE)
    {
        return;
    }

    /* mute bit will reset by terminate timeout*/
    if (ccp_db.active_bit == 0)
    {
        APP_PRINT_INFO0("ccp_handle_telephony_mute_timeout: this call will terminate later");
        return;
    }

#if DONGLE_LE_AUDIO
    extern void app_le_audio_set_mic_mute(uint8_t mic_mute);
    /*FIXME: MICS should send here*/
    app_le_audio_set_mic_mute(ccp_db.mute_bit);
#endif
}

static void teams_ccp_timeout_cback(uint8_t timer_id, uint16_t timer_chann)
{
    APP_PRINT_INFO2("teams_ccp_timeout_cback id %x, chnl %x", timer_id, timer_chann);

    switch (timer_id)
    {
    case CCP_TIMER_ID_INCOMING:
        {
            app_stop_timer(&ccp_db.ccp_incoming_timer);
            ccp_handle_incoming_timeout();
        }
        break;
    case CCP_TIMER_ID_TERMINATE:
        {
            app_stop_timer(&ccp_db.ccp_terminate_timer);
            ccp_handle_terminate_timeout();
        }
        break;
    case CCP_TIMER_ID_OUTGOING:
        {
            app_stop_timer(&ccp_db.ccp_outgoing_timer);
            ccp_handle_outgoing_timeout();
        }
        break;
    case CCP_TIMER_ID_MICS:
        {
            app_stop_timer(&ccp_db.ccp_mics_timer);
            ccp_handle_mics_timeout(timer_chann);
        }
        break;
    case CCP_TIMER_ID_TELEPHONY_MUTE:
        {
            app_stop_timer(&ccp_db.ccp_telephony_mute_timer);
            ccp_handle_telephony_mute_timeout(timer_chann);
        }
        break;
    default:
        break;
    }
}


static T_APP_RESULT ccp_handle_op_accept(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    uint8_t call_id = 0;
    T_TELEPHONY_HID_INPUT ctrl_code = {0x0};
    T_CALL_DB *p_call = NULL;

    call_id = p_ccp_val->param.accept_opcode_call_index;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return APP_RESULT_APP_ERR;
    }

    APP_PRINT_INFO3("ccp_handle_op_accept call_id %x ring_id %x state %x",
                    call_id, ccp_db.ring_call_id, p_call->call_state);
    if (ccp_db.ring_call_id == call_id)
    {
        if (p_call->call_state == TBS_CALL_STATE_INCOMING)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            ctrl_code.mute = ccp_db.mute_bit ? true : false;
            ctrl_code.hook_switch = true;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            return APP_RESULT_SUCCESS;
        }
    }

    return APP_RESULT_APP_ERR;
}

static T_APP_RESULT ccp_handle_op_terminate(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    uint8_t call_id = 0;
    T_TELEPHONY_HID_INPUT ctrl_code = {0x0};
    T_CALL_DB *p_call = NULL;

    call_id = p_ccp_val->param.terminate_opcode_call_index;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return APP_RESULT_APP_ERR;
    }

    APP_PRINT_INFO5("ccp_handle_op_terminate call_id %x  state %x, ring_id %x active_id %x hold_id %x",
                    call_id, p_call->call_state, ccp_db.ring_call_id,
                    ccp_db.active_call_id, ccp_db.hold_call_id);
    /* ring call priority is high than active call */
    if (ccp_db.ring_call_id == call_id)
    {
        if (p_call->call_state == TBS_CALL_STATE_INCOMING)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
#if (USB_HID_BUTTON_SUPPORT == 1)
            ctrl_code.button = true;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
            ctrl_code.button = false;
#endif
            ctrl_code.hook_switch = false;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            return APP_RESULT_SUCCESS;
        }
    }
    else if (ccp_db.active_call_id == call_id)
    {
        if (p_call->call_state == TBS_CALL_STATE_ACTIVE)
        {
            if (ccp_db.ccp_start_by_upstream)
            {
                APP_PRINT_INFO1("ccp_handle_op_terminate id %x started by upstream", call_id);
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
                app_usb_hid_send_consumer_ctrl_key_down(KEY_PLAY_PAUSE);
                app_usb_hid_send_consumer_ctrl_key_down(KEY_RELEASE);
                ctrl_code.hook_switch = true;
                app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
                ctrl_code.hook_switch = false;
                app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            }
            else
            {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
#if (USB_HID_BUTTON_SUPPORT == 1)
                ctrl_code.button = true;
                app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
                ctrl_code.button = false;
#endif
                ctrl_code.mute = ccp_db.mute_bit ? true : false;
                ctrl_code.hook_switch = true;
                app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
                ctrl_code.hook_switch = false;
                app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            }

            return APP_RESULT_SUCCESS;
        }
    }
    else if (ccp_db.hold_call_id == call_id)
    {
        if (p_call->call_state == TBS_CALL_STATE_LOCALLY_HELD)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            ctrl_code.hook_switch = false;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            return APP_RESULT_SUCCESS;
        }
    }

    return APP_RESULT_APP_ERR;
}

static T_APP_RESULT ccp_handle_op_local_hold(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
#if USB_HID_HOLD_SUPPORT
    uint8_t call_id = 0;
    T_CALL_DB *p_call = NULL;
    T_TELEPHONY_HID_INPUT ctrl_code = {0x0};

    call_id = p_ccp_val->param.local_hold_opcode_call_index;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return APP_RESULT_APP_ERR;
    }

    APP_PRINT_INFO3("ccp_handle_op_local_hold call_id %x active_id %x state %x",
                    call_id, ccp_db.active_call_id, p_call->call_state);
    if (ccp_db.active_call_id == call_id)
    {
        if (p_call->call_state == TBS_CALL_STATE_ACTIVE)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT

            ctrl_code.hold = true;
            ctrl_code.flash = true;
            ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
            ctrl_code.mute = ccp_db.mute_bit ? true : false;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            return APP_RESULT_SUCCESS;
        }
    }
#endif
    return APP_RESULT_APP_ERR;
}

static T_APP_RESULT ccp_handle_op_local_retrieve(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
#if USB_HID_HOLD_SUPPORT
    uint8_t call_id = 0;
    T_TELEPHONY_HID_INPUT ctrl_code = {0x0};
    T_CALL_DB *p_call = NULL;

    call_id = p_ccp_val->param.local_retrieve_opcode_call_index;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return APP_RESULT_APP_ERR;
    }

    APP_PRINT_INFO3("ccp_handle_op_local_retrieve call_id %x hold_id %x state %x",
                    call_id, ccp_db.hold_call_id, p_call->call_state);
    if (ccp_db.hold_call_id == call_id)
    {
        if (p_call->call_state == TBS_CALL_STATE_LOCALLY_HELD)
        {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
            ctrl_code.hold = false;
            ctrl_code.flash = true;
            ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
            ctrl_code.mute = ccp_db.mute_bit ? true : false;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
            ctrl_code.flash = false;
            app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
            return APP_RESULT_SUCCESS;
        }
    }
#endif
    return APP_RESULT_APP_ERR;
}

static T_APP_RESULT ccp_handle_op_originate(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    return APP_RESULT_REJECT;
}

static T_APP_RESULT ccp_handle_op_join(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    return APP_RESULT_REJECT;
}

T_APP_RESULT teams_handle_ccp_op(T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_val)
{
    T_APP_RESULT result = APP_RESULT_APP_ERR;
    if (p_ccp_val == NULL)
    {
        return result;
    }

    APP_PRINT_INFO1("teams_handle_ccp_op opcode %x", p_ccp_val->opcode);
    switch (p_ccp_val->opcode)
    {
    case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ACCEPT:
        {
            result = ccp_handle_op_accept(p_ccp_val);
        }
        break;
    case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_TERMINATE:
        {
            result = ccp_handle_op_terminate(p_ccp_val);
        }
        break;
    case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_HOLD:
        {
            result = ccp_handle_op_local_hold(p_ccp_val);
        }
        break;
    case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_LOCAL_RETRIEVE:
        {
            result = ccp_handle_op_local_retrieve(p_ccp_val);
        }
        break;
    case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_ORIGINATE:
        {
            result = ccp_handle_op_originate(p_ccp_val);
        }
        break;
    case TBS_CALL_CONTROL_POINT_CHAR_OPCODE_JOIN:
        {
            result = ccp_handle_op_join(p_ccp_val);
        }
        break;

    default:
        break;
    }
    return result;
}

static uint8_t teams_alloc_outgoing_call(void)
{
    uint8_t p_call_uri[] = "Outgoing call";
    uint16_t len = strlen("Outgoing call");
    uint8_t call_id = 0;

    call_id = ccp_server_create_call(ccp_db.ccp_id, p_call_uri, len);
    if (call_id == 0)
    {
        APP_PRINT_ERROR0("teams_alloc_outgoing_call fail");
        return false;
    }
    APP_PRINT_INFO1("teams_alloc_outgoing_call id %x", call_id);
    teams_update_call_state(call_id, TBS_CALL_STATE_INCOMING);
    return call_id;
}

static uint8_t teams_alloc_incoming_call(void)
{
    uint8_t p_call_uri[] = "Incoming call";
    uint16_t len = strlen("Incoming call");
    uint8_t call_id = 0;

    call_id = ccp_server_create_call(ccp_db.ccp_id, p_call_uri, len);
    if (call_id == 0)
    {
        APP_PRINT_ERROR0("teams_register_ring_call fail");
        return false;
    }
    APP_PRINT_INFO1("teams_alloc_incoming_call id %x", call_id);
    teams_update_call_state(call_id, TBS_CALL_STATE_INCOMING);
    return call_id;
}

T_TBS_CALL_STATE teams_get_call_state(void)
{
    T_TBS_CALL_STATE state = TBS_CALL_STATE_RFU;
    uint8_t call_id = 0;
    T_CALL_DB *p_call = NULL;

    if (ccp_db.ring_call_id)
    {
        p_call = ccp_get_call_handle_by_call_id(ccp_db.ring_call_id);
        if (p_call)
        {
            call_id = p_call->call_id;
            state = p_call->call_state;
        }
    }
    else if (ccp_db.active_call_id)
    {
        p_call = ccp_get_call_handle_by_call_id(ccp_db.active_call_id);
        if (p_call)
        {
            call_id = p_call->call_id;
            state = p_call->call_state;
        }
    }
    APP_PRINT_TRACE2("teams_get_call_state:id %x state %x", call_id, state);
    return state;
}

static void teams_update_call_state(uint8_t call_id, T_TBS_CALL_STATE state)
{
    T_CALL_DB *p_call = NULL;
    uint8_t call_flags = 0;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        APP_PRINT_ERROR1("teams_update_call_state call_id %d find handle fail", call_id);
        return;
    }

    p_call->call_state = state;
    APP_PRINT_TRACE2("teams_update_call_state call_id %d, state : %d", call_id, state);
    ccp_server_update_call_state_by_call_index(ccp_db.ccp_id, call_id,
                                               state, call_flags & (~(1 << TBS_CALL_FLAGS_BIT_INCOMING_OUTGOING)), true);
}

static bool teams_terminate_call(uint8_t call_id)
{
    T_CALL_DB *p_call = NULL;

    p_call = ccp_get_call_handle_by_call_id(call_id);
    if (!p_call)
    {
        return false;
    }
    APP_PRINT_INFO1("teams_terminate_call id %x", call_id);
    T_CCP_SERVER_TERMINATION_REASON reason;
    reason.call_index = call_id;
    reason.reason_code = TBS_TERMINATION_REASON_CODES_REMOTE_END_CALL;
    ccp_server_terminate_call(ccp_db.ccp_id, &reason);
    ccp_free_call_handle(call_id);

    return true;
}


static void teams_call_handle_active_bit_change(bool active_bit)
{
    APP_PRINT_INFO3("active_bit_change active_bit %x ring_call %x active_call_id %x",
                    active_bit, ccp_db.ring_call_id, ccp_db.active_call_id);
    if (active_bit)
    {
        APP_PRINT_INFO2("active_bit_change ccp_outgoing_timer %x ccp_terminate_timer %x",
                        ccp_db.ccp_outgoing_timer, ccp_db.ccp_terminate_timer);
        /* ring call is accepet */
        if (ccp_db.ring_call_id)
        {
            ccp_db.active_call_id = ccp_db.ring_call_id;
            ccp_db.ring_call_id = 0;
            teams_update_call_state(ccp_db.active_call_id, TBS_CALL_STATE_ACTIVE);
        }
        /* outgoing call */
        else if (ccp_db.active_call_id == 0)
        {
            if (ccp_db.ccp_outgoing_timer == NULL)
            {
                app_start_timer(&ccp_db.ccp_outgoing_timer, "outgoing call",
                                ccp_timer_queue_id, CCP_TIMER_ID_OUTGOING, 0, false, CCP_OUTGOING_TIMEOUT);
            }
        }
        if (ccp_db.ccp_terminate_timer)
        {
            app_stop_timer(&ccp_db.ccp_terminate_timer);
        }
    }
    else
    {
        APP_PRINT_INFO2("active_bit_change ccp_outgoing_timer %x ccp_terminate_timer %x",
                        ccp_db.ccp_outgoing_timer, ccp_db.ccp_terminate_timer);
        if (ccp_db.active_call_id)
        {
            if (ccp_db.ccp_terminate_timer == NULL)
            {
                app_start_timer(&ccp_db.ccp_terminate_timer, "terminate call",
                                ccp_timer_queue_id, CCP_TIMER_ID_TERMINATE, 0, false, CCP_TERMINATE_TIMEOUT);
            }
        }
        else if (ccp_db.ccp_outgoing_timer)
        {
            app_stop_timer(&ccp_db.ccp_outgoing_timer);
        }
    }
}

static void teams_call_handle_ring_bit_change(bool ring_bit)
{
    APP_PRINT_INFO3("ring_bit_change ring_bit %x ring_call %x ccp_incoming_timer %x",
                    ring_bit, ccp_db.ring_call_id, ccp_db.ccp_incoming_timer);
    if (ring_bit)
    {
        /* incoming call */
        if (ccp_db.ring_call_id == 0)
        {
            if (ccp_db.ccp_incoming_timer == NULL)
            {
                app_start_timer(&ccp_db.ccp_incoming_timer, "incoming call",
                                ccp_timer_queue_id, CCP_TIMER_ID_INCOMING, 0, false, CCP_INCOMING_TIMEOUT);
            }
        }
    }
    else
    {
        if (ccp_db.ccp_incoming_timer)
        {
            app_stop_timer(&ccp_db.ccp_incoming_timer);
        }
        if (ccp_db.ring_call_id)
        {
            if (ccp_db.ccp_terminate_timer)
            {
                app_stop_timer(&ccp_db.ccp_terminate_timer);
                if (ccp_db.active_bit)
                {
                    if (teams_terminate_call(ccp_db.active_call_id))
                    {
                        ccp_db.call_num--;
                    }
                    ccp_db.active_call_id = 0;
                }
            }
            app_start_timer(&ccp_db.ccp_terminate_timer, "terminate call",
                            ccp_timer_queue_id, CCP_TIMER_ID_TERMINATE, 0, false, CCP_TERMINATE_TIMEOUT);
        }
    }
}

uint16_t teams_call_handle_ccp_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    uint16_t cb_result = BLE_AUDIO_CB_RESULT_SUCCESS;
    T_CCP_SERVER_WRITE_CALL_CP_IND *p_ccp_result = (T_CCP_SERVER_WRITE_CALL_CP_IND *)buf;

    switch (msg)
    {
    case LE_AUDIO_MSG_CCP_SERVER_WRITE_CALL_CP_IND:
        if (p_ccp_result)
        {
            cb_result = teams_handle_ccp_op(p_ccp_result);
        }
        break;

    case LE_AUDIO_MSG_CCP_SERVER_READ_IND:
        {
            T_CCP_SERVER_READ_IND *p_read_ind = (T_CCP_SERVER_READ_IND *)buf;

            if (p_read_ind)
            {
                T_CCP_SERVER_READ_CFM read_cfm = {0};

                read_cfm.cause = BLE_AUDIO_CB_RESULT_SUCCESS;
                read_cfm.conn_handle = p_read_ind->conn_handle;
                read_cfm.cid = p_read_ind->cid;
                read_cfm.service_id = p_read_ind->service_id;
                read_cfm.char_uuid = p_read_ind->char_uuid;
                read_cfm.offset = p_read_ind->offset;

                switch (p_read_ind->char_uuid)
                {
                case TBS_UUID_CHAR_BEARER_PROVIDER_NAME:
                    {
                        read_cfm.param.bearer_provider_name.p_bearer_provider_name = "RTK dongle";
                        read_cfm.param.bearer_provider_name.bearer_provider_name_len = strlen("RTK dongle");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_BEARER_UCI:
                    {
                        read_cfm.param.bearer_uci.p_bearer_uci = "un001";
                        read_cfm.param.bearer_uci.bearer_uci_len = strlen("un001");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_BEARER_TECHNOLOGY:
                    {
                        uint8_t tech = TBS_BEARER_TECHNOLOGY_CHAR_VALUE_WIFI;

                        read_cfm.param.bearer_technology.p_bearer_technology = &tech;
                        read_cfm.param.bearer_technology.bearer_technology_len = 1;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_BEARER_URI_SCHEMES_SUPPORTED_LIST:
                    {
                        read_cfm.param.bearer_uri_schemes_supported_list.p_bearer_uri_schemes_supported_list = "tel";
                        read_cfm.param.bearer_uri_schemes_supported_list.bearer_uri_schemes_supported_list_len =
                            strlen("tel");

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_CONTENT_CONTROL_ID:
                    {
                        read_cfm.param.content_control_id = APP_CONTENT_CONTROL_ID_CHAR_VALUE_GTBS;

                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                case TBS_UUID_CHAR_STATUS_FLAGS:
                    {
                        /* inband_enable|not silent mode */
                        read_cfm.param.status_flags = 1;
                        cb_result = BLE_AUDIO_CB_RESULT_PENDING;

                        ccp_server_read_cfm(&read_cfm);
                    }
                    break;

                default:
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
    return cb_result;
}

static void teams_call_handle_hold_bit_change(bool hold_bit)
{
    /*FIXME: what the other link will happen when active switch to hold*/
    APP_PRINT_INFO3("hold_bit_change hold_bit %x active_call %x hold_call %x",
                    hold_bit, ccp_db.active_call_id, ccp_db.hold_call_id);
    if (hold_bit)
    {
        if (ccp_db.active_call_id)
        {
            ccp_db.hold_call_id = ccp_db.active_call_id;
            teams_update_call_state(ccp_db.active_call_id, TBS_CALL_STATE_LOCALLY_HELD);
            ccp_db.active_call_id = 0;
        }
    }
    /* FIXME: what */
    else
    {
        if (ccp_db.hold_call_id == 0)
        {
            return;
        }
        if (ccp_db.active_bit)
        {
            ccp_db.active_call_id = ccp_db.hold_call_id;
            teams_update_call_state(ccp_db.hold_call_id, TBS_CALL_STATE_ACTIVE);
            ccp_db.hold_call_id = 0;
        }
        else
        {
            teams_terminate_call(ccp_db.hold_call_id);
            ccp_db.hold_call_id = 0;
        }
    }
}

static void teams_call_handle_mute_bit_change(bool mute_bit)
{
    if (ccp_db.ccp_telephony_mute_timer)
    {
        app_stop_timer(&ccp_db.ccp_telephony_mute_timer);
    }
    app_start_timer(&ccp_db.ccp_telephony_mute_timer, "telephony_mute",
                    ccp_timer_queue_id, CCP_TIMER_ID_TELEPHONY_MUTE, mute_bit, false, CCP_TELEPHONY_MUTE_TIMEOUT);
}

void teams_handle_mics_msg(uint8_t *bd_addr, uint8_t mute)
{
    if (ccp_db.ccp_start_by_upstream)
    {
        APP_PRINT_INFO0("teams_handle_mics_msg start by upstream");
        return;
    }

    APP_PRINT_INFO3("teams_handle_mics_msg: bd_addr %b mute %x, current mute %x",
                    TRACE_BDADDR(bd_addr), mute, ccp_db.mute_bit);

    if (ccp_db.ccp_mics_timer)
    {
        app_stop_timer(&ccp_db.ccp_mics_timer);
    }
    app_start_timer(&ccp_db.ccp_mics_timer, "mics",
                    ccp_timer_queue_id, CCP_TIMER_ID_MICS, mute, false, CCP_MICS_TIMEOUT);
}

void teams_call_handle_telephony_msg(uint16_t msg)
{
    if (ccp_db.ccp_id == 0xFF)
    {
        return;
    }
    bool changed = false;
    uint8_t old_active_bit = ccp_db.active_bit;
    uint8_t old_ring_bit = ccp_db.ring_bit;
    uint8_t old_hold_bit = ccp_db.hold_bit;
    uint8_t old_mute_bit = ccp_db.mute_bit;

    T_TELEPHONY_HID_OUTPUT *p_msg = (T_TELEPHONY_HID_OUTPUT *)&msg;
    T_TELEPHONY_HID_INPUT ctrl_code = {0x0};
    ccp_db.active_bit = p_msg->off_hook;
    ccp_db.ring_bit = p_msg->ring;
    ccp_db.hold_bit = p_msg->hold;
    ccp_db.mute_bit = p_msg->mute;

    APP_PRINT_INFO5("teams_handle_telephony_msg msg %x, old active %d ring %d hold %x mute %x",
                    msg,  old_active_bit, old_ring_bit, old_hold_bit, old_mute_bit);

    T_CALL_DB *p_call = NULL;
    for (uint8_t i = 0; i < CCP_CALL_LIST_MAX; i++)
    {
        p_call = (T_CALL_DB *)&ccp_db.call_list[i];
        APP_PRINT_TRACE4("i %d, call_id %d, call_state %d, call_num:%d", i, p_call->call_id,
                         p_call->call_state, ccp_db.call_num);
    }
    if (ccp_db.ccp_start_by_upstream)
    {
        APP_PRINT_INFO0("teams_handle_telephony_msg: set ccp_start_by_upstream false");
        ccp_db.ccp_start_by_upstream = false;
    }

    if (ccp_db.ccp_start_by_upstream)
    {
        ccp_db.ccp_start_by_upstream = false;
    }

    if (ccp_db.active_bit != old_active_bit)
    {
        changed = true;
        teams_call_handle_active_bit_change(ccp_db.active_bit);
    }

    if (ccp_db.ring_bit != old_ring_bit)
    {
        teams_call_handle_ring_bit_change(ccp_db.ring_bit);
    }

    if (ccp_db.hold_bit != old_hold_bit)
    {
        teams_call_handle_hold_bit_change(ccp_db.hold_bit);
    }

    if (ccp_db.mute_bit != old_mute_bit)
    {
        changed = true;
        teams_call_handle_mute_bit_change(ccp_db.mute_bit);
    }


    /* should reply telephony msg when hook-switch or mute change */
    if (changed)
    {
#if F_APP_USB_HID_SUPPORT && F_APP_USB_SUPPORT
        ctrl_code.mute = ccp_db.mute_bit ? true : false;
        ctrl_code.hook_switch = ccp_db.active_bit ? true : false;
        app_usb_hid_send_telephony_ctrl_code((uint16_t *)&ctrl_code);
#endif
    }
}

uint16_t le_tbs_get_enabled_cccd(void)
{
    return ccp_db.ccp_enabled_cccd;
}

bool le_tbs_get_mute_bit(void)
{
    return ccp_db.mute_bit;
}

void le_tbs_handle_usb_upstream(bool enable)
{
    APP_PRINT_INFO4("le_tbs_handle_usb_upstream enable %x ring_bit %x active_bit %x upstream %x",
                    enable, ccp_db.ring_bit, ccp_db.active_bit, ccp_db.ccp_start_by_upstream);
    if (enable)
    {
        if (!ccp_db.ring_bit && !ccp_db.active_bit)
        {
            ccp_db.ccp_start_by_upstream = true;
            ccp_db.active_bit = true;
            teams_call_handle_active_bit_change(true);
        }
    }
    else
    {
        if (ccp_db.ccp_start_by_upstream)
        {
            ccp_db.ccp_start_by_upstream = false;
            ccp_db.active_bit = false;
            teams_call_handle_active_bit_change(false);
        }
    }
}

bool le_tbs_handle_server_msg(T_LE_AUDIO_MSG msg, void *buf)
{
    T_SERVER_ATTR_CCCD_INFO *p_cccd = NULL;
    uint16_t telephony_output = 0;
    if (LE_AUDIO_MSG_SERVER_ATTR_CCCD_INFO == msg)
    {
        if (!buf)
        {
            return false;
        }
        p_cccd = (T_SERVER_ATTR_CCCD_INFO *)buf;


        if (p_cccd->service_id != ccp_db.ccp_id)
        {
            APP_PRINT_ERROR2("le_tbs_handle_server_msg ccp_id %x, service id %x",
                             ccp_db.ccp_id, p_cccd->service_id);
            return false;
        }

        ccp_db.ccp_enabled_cccd = p_cccd->ccc_bits;
        telephony_output = app_usb_hid_get_telephony_output();
        APP_PRINT_INFO2("le_tbs_handle_server_msg cccd_flag %x output %x", p_cccd->ccc_bits,
                        telephony_output);
        if (ccp_db.ccp_enabled_cccd && telephony_output)
        {
            teams_call_handle_telephony_msg(telephony_output);
        }
        return true;
    }
    return false;
}

void teams_call_control_load_telephony(void)
{
    uint16_t telephony_output = app_usb_hid_get_telephony_output();
    teams_call_handle_telephony_msg(telephony_output);
}

void teams_call_control_init(T_SERVER_ID ccp_id)
{
    T_CCP_SERVER_SET_PARAM param;

    ccp_db.ccp_id = ccp_id;
    if (ccp_id != 0xFF)
    {
        param.char_uuid = TBS_UUID_CHAR_CALL_CONTROL_POINT_OPTIONAL_OPCODES;
        param.param.call_control_point_optional_opcodes = 0x01;
        ccp_server_set_param(ccp_id, &param);
    }

    app_timer_reg_cb(teams_ccp_timeout_cback, &ccp_timer_queue_id);
}
#endif

