#include <string.h>

#include "os_msg.h"
#include "trace.h"

#include "le_bg_conn_mgr.h"
#include "gap.h"
#include "gap_le.h"

#define BG_CONN_MGR_AUTO_RUN    0

T_BG_OP_STATE   op_state = BG_OP_STATE_IDLE;
T_BG_CONN_STATE conn_state = BG_CONN_STATE_IDLE;

void *bg_conn_mgr_msg_queue = NULL;
#define MAX_MSG_NUM     20

uint8_t NULL_ADDR[6] = {0, 0, 0, 0, 0, 0};

T_BG_CONN_ID wl_conn_id = {false, 0};

static uint8_t wl_num = 0;

static void le_bg_conn_mgr_set_op_state(T_BG_OP_STATE new_op_state)
{
    op_state = new_op_state;
}

//static T_BG_OP_STATE le_bg_conn_mgr_get_op_state(void)
//{
//    return op_state;
//}

static void le_bg_conn_mgr_set_conn_state(T_BG_CONN_STATE new_conn_state)
{
    conn_state = new_conn_state;
}

static T_BG_CONN_STATE le_bg_conn_mgr_get_conn_state(void)
{
    return conn_state;
}


static bool le_bg_conn_mgr_is_op_busy(void)
{
    return (op_state == BG_OP_STATE_IDLE) ? false : true;
}

static bool le_bg_conn_mgr_is_conn_busy(void)
{
    return (conn_state == BG_CONN_STATE_IDLE) ? false : true;
}

static bool le_bg_conn_mgr_send_msg(T_BG_CONN_OPCODE op, uint8_t *bd_addr,
                                    uint8_t addr_type)
{
    T_BG_CONN_MSG msg;

    msg.op_code = op;
    msg.addr_type = addr_type;
    memcpy(msg.bd_addr, bd_addr, 6);

    if (os_msg_send(bg_conn_mgr_msg_queue, &msg, 0) != true)
    {
        APP_PRINT_ERROR0("bg_conn_mgr_send_msg: queue full");
        return false;
    }
    return true;
}

static bool le_bg_conn_mgr_send_conn_msg(void)
{
    return le_bg_conn_mgr_send_msg(BG_CONN_MSG_OP_CONN,
                                   NULL_ADDR,
                                   GAP_REMOTE_ADDR_LE_PUBLIC);
}

static bool le_bg_conn_mgr_send_disconn_msg(void)
{
    return le_bg_conn_mgr_send_msg(BG_CONN_MSG_OP_DISCONN,
                                   NULL_ADDR,
                                   GAP_REMOTE_ADDR_LE_PUBLIC);
}

static bool le_bg_conn_mgr_send_wl_add_msg(uint8_t *bd_addr, uint8_t addr_type)
{
    return le_bg_conn_mgr_send_msg(BG_CONN_MSG_OP_WL_ADD,
                                   bd_addr,
                                   addr_type);
}

static bool le_bg_conn_mgr_send_wl_del_msg(uint8_t *bd_addr, uint8_t addr_type)
{
    return le_bg_conn_mgr_send_msg(BG_CONN_MSG_OP_WL_DEL,
                                   bd_addr,
                                   addr_type);
}

static bool le_bg_conn_mgr_send_wl_clr_msg(uint8_t *bd_addr, uint8_t addr_type)
{
    return le_bg_conn_mgr_send_msg(BG_CONN_MSG_OP_WL_CLR,
                                   bd_addr,
                                   addr_type);
}

static bool le_bg_conn_mgr_handle_wl_conn(void)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;

    if (le_bg_conn_mgr_is_conn_busy())
    {
        APP_PRINT_WARN0("bg_conn_mgr_handle_wl_conn: already connecting");
        return false;
    }

    //Set conn params in init !!!
    ret = le_connect(0,
                     NULL_ADDR, GAP_REMOTE_ADDR_LE_PUBLIC,
                     GAP_LOCAL_ADDR_LE_PUBLIC, 0);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("bg_conn_mgr_handle_wl_conn: failed %d", ret);
        return false;
    }

    le_bg_conn_mgr_set_conn_state(BG_CONN_STATE_WAIT_CONN_ID);

    return true;
}

static bool le_bg_conn_mgr_handle_wl_disconn(void)
{
    T_GAP_CAUSE ret = GAP_CAUSE_SUCCESS;

    if (!le_bg_conn_mgr_is_conn_busy())
    {
        APP_PRINT_WARN0("bg_conn_mgr_handle_wl_disconn: already idle");
        return false;
    }

    //This should not happen because of msg queue
    if (!wl_conn_id.is_valid)
    {
        APP_PRINT_WARN0("bg_conn_mgr_handle_wl_disconn: invalid conn_id");
        return false;
    }

    ret = le_disconnect(wl_conn_id.conn_id);

    if (ret != GAP_CAUSE_SUCCESS)
    {
        APP_PRINT_ERROR1("bg_conn_mgr_handle_wl_disconn: failed %d", ret);
        return false;
    }

    le_bg_conn_mgr_set_conn_state(BG_CONN_STATE_WAIT_DICONNECTED);

    return true;
}

static bool le_bg_conn_mgr_hanlde_wl_op(T_BG_CONN_OPCODE op, uint8_t *bd_addr,
                                        uint8_t addr_type)
{
    T_GAP_WHITE_LIST_OP stack_wl_op = GAP_WHITE_LIST_OP_REMOVE;
    T_GAP_CAUSE op_ret = GAP_CAUSE_SUCCESS;
    bool op_valid = false;

    switch (op)
    {
    case BG_CONN_MSG_OP_WL_CLR:
        {
            stack_wl_op = GAP_WHITE_LIST_OP_CLEAR;
            op_valid = true;
        }
        break;

    case BG_CONN_MSG_OP_WL_ADD:
        {
            stack_wl_op = GAP_WHITE_LIST_OP_ADD;
            op_valid = true;
        }
        break;

    case BG_CONN_MSG_OP_WL_DEL:
        {
            stack_wl_op = GAP_WHITE_LIST_OP_REMOVE;
            op_valid = true;
        }
        break;

    default:
        break;
    }

    if (op_valid)
    {
        op_ret = le_modify_white_list(stack_wl_op, bd_addr, (T_GAP_REMOTE_ADDR_TYPE)addr_type);
        if (op_ret != GAP_CAUSE_SUCCESS)
        {
            APP_PRINT_ERROR1("bg_conn_mgr_hanlde_wl_op: %d", op_ret);
            return false;
        }

        return true;
    }

    return false;
}


static bool le_bg_conn_mgr_handle_msg(T_BG_CONN_MSG msg)
{
    bool ret = false;

    switch (msg.op_code)
    {

    case BG_CONN_MSG_OP_CONN:
        ret = le_bg_conn_mgr_handle_wl_conn();
        break;

    case BG_CONN_MSG_OP_DISCONN:
        ret = le_bg_conn_mgr_handle_wl_disconn();
        break;

    case BG_CONN_MSG_OP_WL_CLR:
    case BG_CONN_MSG_OP_WL_ADD:
    case BG_CONN_MSG_OP_WL_DEL:
        ret = le_bg_conn_mgr_hanlde_wl_op(msg.op_code, msg.bd_addr, msg.addr_type);
        break;

    default:
        break;
    }

    return ret;
}

static void le_bg_conn_mgr_check_msg(void)
{
    T_BG_CONN_MSG msg;

    if (os_msg_recv(bg_conn_mgr_msg_queue, &msg, 0) == true)
    {
        APP_PRINT_INFO3("le_bg_conn_mgr_check_msg: op %d, addr_type %d, addr %s",
                        msg.op_code, msg.addr_type, TRACE_BDADDR(msg.bd_addr));

        if (!le_bg_conn_mgr_handle_msg(msg))
        {
            le_bg_conn_mgr_check_msg();
        }
        else
        {
            le_bg_conn_mgr_set_op_state(BG_OP_STATE_BUSY);
        }
    }
    else
    {
#if BG_CONN_MGR_AUTO_RUN
        //no more msg, check to start conn
        if ((wl_num > 0) && (!le_bg_conn_mgr_is_conn_busy()))
        {
            le_bg_conn_mgr_create_conn();
        }
#endif
    }
}

void le_bg_conn_mgr_handle_wl_op_result(T_GAP_WHITE_LIST_OP op, uint16_t cause)
{
    if (cause == GAP_CAUSE_SUCCESS)
    {
        switch (op)
        {
        case GAP_WHITE_LIST_OP_CLEAR:
            wl_num = 0;
            break;

        case GAP_WHITE_LIST_OP_ADD:
            wl_num++;
            break;

        case GAP_WHITE_LIST_OP_REMOVE:
            wl_num--;
            break;

        default:
            APP_PRINT_WARN1("le_bg_conn_mgr_handle_wl_op_result: unknown op %d", op);
            break;
        }
    }
    else
    {
        APP_PRINT_WARN2("le_bg_conn_mgr_handle_wl_op_result: failed %d %04x", op, cause);
    }

    APP_PRINT_INFO1("le_bg_conn_mgr_handle_wl_op_result: num of wl entry %d", wl_num);

    le_bg_conn_mgr_set_op_state(BG_OP_STATE_IDLE);

    le_bg_conn_mgr_check_msg();
}

void le_bg_conn_mgr_handle_conn_state_change(uint8_t conn_id,
                                             T_GAP_CONN_STATE new_state, uint16_t disc_cause)
{
    bool action = false;;

    APP_PRINT_INFO6("le_bg_conn_mgr_handle_conn_state_change: %d %d %04x %d %d %d",
                    conn_id, new_state, disc_cause,
                    wl_conn_id.conn_id, wl_conn_id.is_valid,
                    le_bg_conn_mgr_get_conn_state());

    switch (new_state)
    {
    case GAP_CONN_STATE_DISCONNECTED:
        {
            if (//(le_bg_conn_mgr_get_conn_state() == BG_CONN_STATE_WAIT_DICONNECTED) &&
                (wl_conn_id.conn_id == conn_id) &&
                wl_conn_id.is_valid)
            {
                APP_PRINT_INFO1("le_bg_conn_mgr_handle_conn_state_change: %d disconnected", conn_id);

                wl_conn_id.is_valid = false;
                le_bg_conn_mgr_set_conn_state(BG_CONN_STATE_IDLE);

                action = true;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTING:
        {
            if (!wl_conn_id.is_valid &&
                (le_bg_conn_mgr_get_conn_state() == BG_CONN_STATE_WAIT_CONN_ID))
            {
                APP_PRINT_INFO1("le_bg_conn_mgr_handle_conn_state_change: %d connecting", conn_id);

                wl_conn_id.conn_id = conn_id;
                wl_conn_id.is_valid = true;
                le_bg_conn_mgr_set_conn_state(BG_CONN_STATE_BUSY);

                action = true;
            }
        }
        break;

    case GAP_CONN_STATE_CONNECTED:
        {
            if ((le_bg_conn_mgr_get_conn_state() == BG_CONN_STATE_BUSY)
                && (wl_conn_id.conn_id == conn_id)
                && wl_conn_id.is_valid)
            {
                APP_PRINT_INFO1("le_bg_conn_mgr_handle_conn_state_change: %d connected", conn_id);

                wl_conn_id.is_valid = false;
                le_bg_conn_mgr_set_conn_state(BG_CONN_STATE_IDLE);

                action = true;
            }
        }
        break;

    default:
        break;
    }

    if (action)
    {
        le_bg_conn_mgr_set_op_state(BG_OP_STATE_IDLE);
        le_bg_conn_mgr_check_msg();
    }
}

bool le_bg_conn_mgr_create_conn(void)
{
    if (le_bg_conn_mgr_send_conn_msg())
    {
        if (!le_bg_conn_mgr_is_op_busy())
        {
            //trigger os_msg_recv
            le_bg_conn_mgr_check_msg();
        }

        return true;
    }
    else
    {
        APP_PRINT_ERROR0("le_bg_conn_mgr_create_conn: failed to conn");
        return false;
    }
}


bool le_bg_conn_mgr_disconn(void)
{
    if (wl_conn_id.is_valid)
    {
        if (le_bg_conn_mgr_send_disconn_msg())
        {
            if (!le_bg_conn_mgr_is_op_busy())
            {
                //trigger os_msg_recv
                le_bg_conn_mgr_check_msg();
            }
            return true;
        }
        else
        {
            APP_PRINT_ERROR1("le_bg_conn_mgr_disconn: failed to disconn %d", wl_conn_id.conn_id);
        }
    }
    else
    {
        APP_PRINT_WARN0("le_bg_conn_mgr_disconn: not connecting");
    }

    return false;
}
bool le_bg_conn_mgr_get_conn_id(uint8_t *conn_id)
{
    if (wl_conn_id.is_valid)
    {
        *conn_id = wl_conn_id.conn_id;
        return true;
    }

    return false;
}

bool le_bg_conn_mgr_wl_add(uint8_t *bd_addr, uint8_t addr_type)
{
#if BG_CONN_MGR_AUTO_RUN
    if (le_bg_conn_mgr_is_op_busy() ||
        le_bg_conn_mgr_is_conn_busy())
    {
        le_bg_conn_mgr_send_disconn_msg();
    }
#endif
    if (le_bg_conn_mgr_send_wl_add_msg(bd_addr, addr_type))
    {
        if (!le_bg_conn_mgr_is_op_busy())
        {
            //trigger os_msg_recv
            le_bg_conn_mgr_check_msg();
        }

        return true;
    }
    else
    {
        APP_PRINT_ERROR1("le_bg_conn_mgr_wl_add: failed to add %s", TRACE_BDADDR(bd_addr));
    }

    return false;
}

bool le_bg_conn_mgr_wl_del(uint8_t *bd_addr, uint8_t addr_type)
{
#if BG_CONN_MGR_AUTO_RUN
    if (le_bg_conn_mgr_is_op_busy() ||
        le_bg_conn_mgr_is_conn_busy())
    {
        le_bg_conn_mgr_send_disconn_msg();
    }
#endif
    if (le_bg_conn_mgr_send_wl_del_msg(bd_addr, addr_type))
    {
        if (!le_bg_conn_mgr_is_op_busy())
        {
            //trigger os_msg_recv
            le_bg_conn_mgr_check_msg();
        }

        return true;
    }
    else
    {
        APP_PRINT_ERROR1("le_bg_conn_mgr_wl_del: failed to remove %s", TRACE_BDADDR(bd_addr));
    }

    return false;
}

bool le_bg_conn_mgr_wl_clr(uint8_t *bd_addr, uint8_t addr_type)
{
#if BG_CONN_MGR_AUTO_RUN
    if (le_bg_conn_mgr_is_op_busy() ||
        le_bg_conn_mgr_is_conn_busy())
    {
        le_bg_conn_mgr_send_disconn_msg();
    }
#endif
    if (le_bg_conn_mgr_send_wl_clr_msg(bd_addr, addr_type))
    {
        if (!le_bg_conn_mgr_is_op_busy())
        {
            //trigger os_msg_recv
            le_bg_conn_mgr_check_msg();
        }

        return true;
    }
    else
    {
        APP_PRINT_ERROR0("le_bg_conn_mgr_wl_clr: failed clear");
    }

    return false;
}

static void le_bg_conn_mgr_queue_init(void)
{
    if (os_msg_queue_create(&bg_conn_mgr_msg_queue, 'bgQ', MAX_MSG_NUM,
                            sizeof(T_BG_CONN_MSG)) != true)
    {
        APP_PRINT_ERROR0("ble_extend_adv_queue_init: os_msg_queue_create fail");
    }
}

void le_bg_conn_mgr_init(void)
{
    le_bg_conn_mgr_queue_init();
}
