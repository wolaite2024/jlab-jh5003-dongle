#ifndef _LE_BG_CONN_MGR_H_
#define _LE_BG_CONN_MGR_H_

#include <stdint.h>
#include <stdbool.h>

#include "gap_le_types.h"
#include "gap_conn_le.h"

typedef enum
{
    BG_CONN_STATE_IDLE,
    BG_CONN_STATE_WAIT_CONN_ID,
    BG_CONN_STATE_BUSY,
    BG_CONN_STATE_WAIT_DICONNECTED,
} T_BG_CONN_STATE;

typedef enum
{
    BG_OP_STATE_IDLE,
    BG_OP_STATE_BUSY,
} T_BG_OP_STATE;

typedef enum
{
    BG_CONN_MSG_OP_CONN,
    BG_CONN_MSG_OP_DISCONN,
    BG_CONN_MSG_OP_WL_CLR,
    BG_CONN_MSG_OP_WL_ADD,
    BG_CONN_MSG_OP_WL_DEL,
} T_BG_CONN_OPCODE;

typedef struct
{
    T_BG_CONN_OPCODE    op_code;
    uint8_t                     addr_type;
    uint8_t                     bd_addr[6];
} T_BG_CONN_MSG;

typedef struct
{
    bool        is_valid;
    uint8_t     conn_id;
} T_BG_CONN_ID;

void le_bg_conn_mgr_handle_wl_op_result(T_GAP_WHITE_LIST_OP op, uint16_t cause);

void le_bg_conn_mgr_handle_conn_state_change(uint8_t conn_id,
                                             T_GAP_CONN_STATE new_state, uint16_t disc_cause);

bool le_bg_conn_mgr_create_conn(void);
bool le_bg_conn_mgr_disconn(void);
bool le_bg_conn_mgr_get_conn_id(uint8_t *conn_id);

bool le_bg_conn_mgr_wl_add(uint8_t *bd_addr, uint8_t addr_type);
bool le_bg_conn_mgr_wl_del(uint8_t *bd_addr, uint8_t addr_type);
bool le_bg_conn_mgr_wl_clr(uint8_t *bd_addr, uint8_t addr_type);

void le_bg_conn_mgr_init(void);

#endif
