#ifndef _APP_LE_EXT_FTL_H_
#define _APP_LE_EXT_FTL_H_
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <app_msg.h>
#include <profile_client.h>
#include "os_queue.h"
#include "profile_server_def.h"
#include "profile_server.h"

#include "ual_bluetooth.h"

#define LEA_EXT_FTL_PARTITION_NAME       "LEA_EXT_FTL"
#define LEA_EXT_FTL_SIZE                 0x200
#define LEA_EXT_FTL_BLOCK_LEN            64

#define LEA_DEV_RECORD_MAX              4
#define LEA_FTL_SYNC_WORD               0x5AA5A55A

#define CSIS_HASH_LEN                   (3)

#define LEA_BROADCAST_ID_OFFSET         (0x100)
#define LEA_BROADCAST_ID_SIZE           (4)

typedef struct
{
    uint32_t sync_word;
    uint8_t bd_addr[BD_ADDR_LEN];
    uint8_t bd_type;
    uint8_t csis_support;
    uint8_t csis_hash[CSIS_HASH_LEN];
    uint8_t csis_size;
} T_LEA_FTL_DEV;

typedef struct
{
    uint32_t sync_word;
    uint8_t broadcast_id[3];
    uint8_t rsv;
} T_LEA_BST;



bool lea_ext_ftl_init(void);

bool lea_ext_save_le_dev(T_LEA_FTL_DEV *p_dev);

bool lea_ext_find_dev_by_hash(uint8_t *hash, uint8_t dev_idx, T_LEA_FTL_DEV *p_dev);
//bool lea_ext_find_hash_num(uint8_t* hash, uint8_t* num);

bool lea_ext_find_dev_by_addr(uint8_t *bdaddr, uint8_t addr_type, T_LEA_FTL_DEV *p_dev,
                              uint32_t *p_offset);

bool lea_ext_save_broadcast_id(uint8_t *broadcast_id);

bool lea_ext_read_broadcast_id(uint8_t *broadcast_id);

bool lea_ext_clear_hash(void);

#endif
