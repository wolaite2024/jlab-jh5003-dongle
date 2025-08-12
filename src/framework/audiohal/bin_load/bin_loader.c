/*
 * Copyright (c) 2021, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "os_msg.h"
#include "os_mem.h"
#include "os_queue.h"
#include "trace.h"
#include "section.h"
#include "bin_loader.h"

/* TODO Remove Start */
#include "app_msg.h"
#include "sys_mgr.h"
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
#include <string.h>
#endif

extern void *hEventQueueHandleAu;
void *hBINLOADERQueueHandleAu;
/* TODO Remove End */

#define BIN_LOADER_MSG_MAX_NUM     (0x08)

typedef enum t_bin_loader_cb_event
{
    BIN_LOADER_CB_EVENT_NONE,
    BIN_LOADER_CB_LOAD_FINISH_STATE_MSG,
    BIN_LOADER_CB_LOAD_PART_STATE_MSG,
    BIN_LOADER_CB_EVENT_MAX,
} T_BIN_LOADER_CB_EVENT;

typedef enum t_bin_loader_state
{
    BIN_LOADER_STATE_IDLE,
    BIN_LOADER_STATE_BUSY,
    BIN_LOADER_STATE_MAX,
} T_BIN_LOADER_STATE;

typedef struct t_bin_loader_session
{
    struct t_bin_loader_session *p_next;
    uint32_t                     id;
    P_BIN_LOADER_EXEC    exec;
    P_BIN_LOADER_VERIFY verify;
    P_BIN_LOADER_CBACK  cback;
} T_BIN_LOADER_SESSION;

typedef struct t_bin_loader_token
{
    struct t_bin_loader_token *p_next;
    T_BIN_LOADER_SESSION      *session;
    uint32_t                   id;
    void                      *context;
} T_BIN_LOADER_TOKEN;

typedef struct t_bin_loader_db
{
    T_OS_QUEUE          session_list;
    T_OS_QUEUE          token_list;
    T_BIN_LOADER_STATE  state;
} T_BIN_LOADER_DB;

typedef struct t_bin_loader_msg
{
    uint8_t     msg_type;
    uint16_t    data_len;
    void        *p_data;
} T_BIN_LOADER_MSG;

static T_BIN_LOADER_DB *bin_loader_db = NULL;

typedef void (*BIN_LOADER_DRIVER_CB)(void);

extern void bin_loader_driver_reg_cb(BIN_LOADER_DRIVER_CB check_finish, BIN_LOADER_DRIVER_CB finish,
                                     BIN_LOADER_DRIVER_CB err);

RAM_TEXT_SECTION
void bin_loader_send_isr_msg(T_BIN_LOADER_MSG *msg)
{
    uint8_t event = EVENT_BIN_LOADER_MSG;
    int32_t ret = 0;

    msg->msg_type = BIN_LOADER_CB_LOAD_PART_STATE_MSG;

    if (hEventQueueHandleAu == NULL)
    {
        ret = 1;
        goto fail;
    }

    if (os_msg_send(hBINLOADERQueueHandleAu, msg, 0) == false)
    {
        ret = 2;
        goto fail;
    }

    if (os_msg_send(hEventQueueHandleAu, &event, 0) == false)
    {
        ret = 3;
        goto fail;
    }

    return;

fail:
    LOADER_PRINT_ERROR1("bin_loader_send_isr_msg: failed %d", -ret);
}

RAM_TEXT_SECTION
void bin_loader_send_msg(T_BIN_LOADER_MSG *msg)
{
    uint8_t event = EVENT_BIN_LOADER_MSG;
    int32_t ret = 0;

    msg->msg_type = BIN_LOADER_CB_LOAD_FINISH_STATE_MSG;

    if (hEventQueueHandleAu == NULL)
    {
        ret = 1;
        goto fail;
    }

    if (os_msg_send(hBINLOADERQueueHandleAu, msg, 0) == false)
    {
        ret = 2;
        goto fail;
    }

    if (os_msg_send(hEventQueueHandleAu, &event, 0) == false)
    {
        ret = 3;
        goto fail;
    }

    return;

fail:
    LOADER_PRINT_ERROR1("bin_loader_send_msg: failed %d", -ret);
}

RAM_TEXT_SECTION void bin_loader_driver_cb_check_finish(void)
{
    T_BIN_LOADER_MSG bin_loader_msg;

    bin_loader_send_isr_msg(&bin_loader_msg);
}

RAM_TEXT_SECTION void bin_loader_driver_cb_finish(void)
{
    T_BIN_LOADER_MSG bin_loader_msg;
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
    T_BIN_LOADER_EVENT evt = BIN_LOADER_EVENT_SUCCESS;
    bin_loader_msg.p_data = os_mem_zalloc2(sizeof(T_BIN_LOADER_EVENT));
    memcpy(bin_loader_msg.p_data, &evt, sizeof(T_BIN_LOADER_EVENT));
    bin_loader_msg.data_len = sizeof(T_BIN_LOADER_EVENT);
#endif
    bin_loader_send_msg(&bin_loader_msg);
}

RAM_TEXT_SECTION void bin_loader_driver_cb_err(void)
{
    T_BIN_LOADER_MSG bin_loader_msg;
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
    T_BIN_LOADER_EVENT evt = BIN_LOADER_EVENT_EXEC_FAIL;
    bin_loader_msg.p_data = os_mem_zalloc2(sizeof(T_BIN_LOADER_EVENT));
    memcpy(bin_loader_msg.p_data, &evt, sizeof(T_BIN_LOADER_EVENT));
    bin_loader_msg.data_len = sizeof(T_BIN_LOADER_EVENT);
#endif
    bin_loader_send_msg(&bin_loader_msg);
}

bool bin_loader_session_check(T_BIN_LOADER_SESSION *session)
{
    if (session == NULL)
    {
        return false;
    }

    if (os_queue_search(&bin_loader_db->session_list, session) == true)
    {
        return true;
    }

    return false;
}

T_BIN_LOADER_SESSION_HANDLE bin_loader_session_create(P_BIN_LOADER_EXEC exec,
                                                      P_BIN_LOADER_VERIFY verify,
                                                      P_BIN_LOADER_CBACK cback)
{
    T_BIN_LOADER_SESSION *session;
    int32_t               ret = 0;

    session = os_mem_alloc2(sizeof(T_BIN_LOADER_SESSION));
    if (session == NULL)
    {
        ret = 1;
        goto fail_alloc_session;
    }

    session->exec = exec;
    session->verify = verify;
    session->cback = cback;

    bin_loader_driver_reg_cb(bin_loader_driver_cb_check_finish,
                             bin_loader_driver_cb_finish,
                             bin_loader_driver_cb_err);

    os_queue_in(&bin_loader_db->session_list, session);

    return (T_BIN_LOADER_SESSION_HANDLE)session;

fail_alloc_session:
    LOADER_PRINT_ERROR1("bin_loader_session_create: failed %d", -ret);
    return NULL;
}

bool bin_loader_session_destory(T_BIN_LOADER_SESSION_HANDLE handle)
{
    T_BIN_LOADER_SESSION *session;
    T_BIN_LOADER_TOKEN   *curr_token;
    T_BIN_LOADER_TOKEN   *next_token;

    session = (T_BIN_LOADER_SESSION *)handle;
    if (bin_loader_session_check(session) == false)
    {
        return false;
    }

    curr_token = os_queue_peek(&bin_loader_db->token_list, 0);
    while (curr_token != NULL)
    {
        next_token = curr_token->p_next;

        if (curr_token->session == session)
        {
            os_msg_queue_delete(curr_token);
        }

        curr_token = next_token;
    }

    os_queue_delete(&bin_loader_db->session_list, session);
    os_mem_free(session);

    return true;
}

bool bin_loader_token_issue(T_BIN_LOADER_SESSION_HANDLE  handle,
                            uint32_t                     id,
                            void                        *context)
{
    T_BIN_LOADER_SESSION *session;
    T_BIN_LOADER_TOKEN *token;
    int32_t             ret = 0;

    session = (T_BIN_LOADER_SESSION *)handle;

    LOADER_PRINT_TRACE3("bin_loader_token_issue: handle %p, id 0x%08x, context %p",
                        handle, id, context);

    if (bin_loader_session_check(session) == false)
    {
        ret = 1;
        goto fail_check_session;
    }

    token = os_mem_alloc2(sizeof(T_BIN_LOADER_TOKEN));
    if (token == NULL)
    {
        ret = 2;
        goto fail_alloc_token;
    }

    token->session = session;
    token->id      = id;
    token->context = context;
    os_queue_in(&bin_loader_db->token_list, token);

    if (bin_loader_db->state == BIN_LOADER_STATE_IDLE)
    {
        bin_loader_db->state = BIN_LOADER_STATE_BUSY;
        session->exec(session, id, context);
    }

    return true;

fail_alloc_token:
fail_check_session:
    LOADER_PRINT_TRACE2("bin_loader_token_issue: handle %p, failed %d", handle, -ret);
    return false;
}

void bin_loader_msg_handler(void)
{
    T_BIN_LOADER_MSG msg;
    T_BIN_LOADER_TOKEN *token;

    if (os_msg_recv(hBINLOADERQueueHandleAu, &msg, 0) == true)
    {
        switch (msg.msg_type)
        {
        case BIN_LOADER_CB_LOAD_PART_STATE_MSG:
            {
                token = os_queue_peek(&bin_loader_db->token_list, 0);
                if (token != NULL)
                {
                    LOADER_PRINT_TRACE3("bin_loader_msg_handler: msg_type 0x%02x, state 0x%02x, token %p",
                                        msg.msg_type, bin_loader_db->state, token);

                    bin_loader_db->state = BIN_LOADER_STATE_IDLE;
                    token->session->verify(token->session, token->id, token->context);
                }                                 //os_queue_in(&bin_loader_db->token_list, token);
            }
            break;

        case BIN_LOADER_CB_LOAD_FINISH_STATE_MSG:
            {
                token = os_queue_out(&bin_loader_db->token_list);
                if (token != NULL)
                {

                    LOADER_PRINT_TRACE3("bin_loader_msg_handler: msg_type 0x%02x, state 0x%02x, token %p",
                                        msg.msg_type, bin_loader_db->state, token);
                    bin_loader_db->state = BIN_LOADER_STATE_IDLE;
#if (TARGET_RTL8773D == 1)/*https://jira.realtek.com/browse/BBPRO3RD-718*/
                    T_BIN_LOADER_EVENT evt = *((T_BIN_LOADER_EVENT *)msg.p_data);
                    token->session->cback(token->session, evt, token->context);
                    os_mem_free(msg.p_data);
#else
                    token->session->cback(token->session, BIN_LOADER_EVENT_SUCCESS, token->context);
#endif
                    os_mem_free(token);
                    if (bin_loader_db->state == BIN_LOADER_STATE_IDLE)
                    {
                        token = os_queue_peek(&bin_loader_db->token_list, 0);
                        if (token != NULL)
                        {
                            bin_loader_db->state = BIN_LOADER_STATE_BUSY;
                            token->session->exec(token->session, token->id, token->context);
                        }
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

bool bin_loader_init(void)
{
    int32_t ret = 0;

    if (bin_loader_db != NULL)
    {
        ret = 1;
        goto fail_check_init;
    }

    bin_loader_db = os_mem_alloc2(sizeof(T_BIN_LOADER_DB));
    if (bin_loader_db == NULL)
    {
        ret = 2;
        goto fail_alloc_db;
    }

    if (os_msg_queue_create(&hBINLOADERQueueHandleAu, "binloadQ",
                            BIN_LOADER_MSG_MAX_NUM,
                            sizeof(T_BIN_LOADER_MSG)) == false)
    {
        ret = 3;
        goto fail_create_queue;
    }

    sys_mgr_event_register(EVENT_BIN_LOADER_MSG, bin_loader_msg_handler);

    bin_loader_db->state = BIN_LOADER_STATE_IDLE;
    os_queue_init(&bin_loader_db->session_list);
    os_queue_init(&bin_loader_db->token_list);

    return true;

fail_create_queue:
    os_mem_free(bin_loader_db);
    bin_loader_db = NULL;
fail_alloc_db:
fail_check_init:
    LOADER_PRINT_ERROR1("bin_loader_init: failed %d", -ret);
    return false;
}

void bin_loader_deinit(void)
{
    if (bin_loader_db != NULL)
    {
        os_mem_free(bin_loader_db);
        bin_loader_db = NULL;
    }
}
