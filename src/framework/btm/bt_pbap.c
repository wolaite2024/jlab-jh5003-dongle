/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_PBAP_SUPPORT == 1)

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_queue.h"
#include "trace.h"
#include "bt_types.h"
#include "obex.h"
#include "bt_mgr_int.h"
#include "bt_pbap_int.h"
#include "bt_pbap.h"

/* Application parameters(TLV format) Tag for PBAP */
#define BT_PBAP_TAG_ID_ORDER                        0x01
#define BT_PBAP_TAG_ID_SEARCH_VALUE                 0x02
#define BT_PBAP_TAG_ID_SEARCH_PROP                  0x03
#define BT_PBAP_TAG_ID_MAX_LIST_COUNT               0x04
#define BT_PBAP_TAG_ID_START_OFFSET                 0x05
#define BT_PBAP_TAG_ID_FILTER                       0x06
#define BT_PBAP_TAG_ID_FORMAT                       0x07
#define BT_PBAP_TAG_ID_PB_SIZE                      0x08
#define BT_PBAP_TAG_ID_NEW_MISS_CALL                0x09
#define BT_PBAP_TAG_ID_PRIMARY_VERSION_COUNTER      0x0A
#define BT_PBAP_TAG_ID_SECONDARY_VERSION_COUNTER    0x0B
#define BT_PBAP_TAG_ID_VCARD_SELECTOR               0x0C
#define BT_PBAP_TAG_ID_DATABASE_IDENTIFIER          0x0D
#define BT_PBAP_TAG_ID_VCARD_SELECTOR_OPERATOR      0x0E
#define BT_PBAP_TAG_ID_RESET_NEW_MISSED_CALLS       0x0F
#define BT_PBAP_TAG_ID_PBAP_FEATURE                 0x10

#define BT_PBAP_TAG_ID_ORDER_LEN                        1
#define BT_PBAP_TAG_ID_SEARCH_PROP_LEN                  1
#define BT_PBAP_TAG_ID_MAX_LIST_COUNT_LEN               2
#define BT_PBAP_TAG_ID_START_OFFSET_LEN                 2
#define BT_PBAP_TAG_ID_FILTER_LEN                       8
#define BT_PBAP_TAG_ID_FORMAT_LEN                       1
#define BT_PBAP_TAG_ID_PB_SIZE_LEN                      2
#define BT_PBAP_TAG_ID_NEW_MISS_CALL_LEN                1
#define BT_PBAP_TAG_ID_PRIMARY_VERSION_COUNTER_LEN      16
#define BT_PBAP_TAG_ID_SECONDARY_VERSION_COUNTER_LEN    16
#define BT_PBAP_TAG_ID_VCARD_SELECTOR_LEN               8
#define BT_PBAP_TAG_ID_DATABASE_IDENTIFIER_LEN          16
#define BT_PBAP_TAG_ID_VCARD_SELECTOR_OPERATOR_LEN      1
#define BT_PBAP_TAG_ID_RESET_NEW_MISSED_CALLS_LEN       1
#define BT_PBAP_TAG_ID_PBAP_FEATURE_LEN                 4

#define BT_PBAP_TAG_ID_PBAP_FEAT_DOWNLOAD                           0x00000001
#define BT_PBAP_TAG_ID_PBAP_FEAT_BROWSING                           0x00000002
#define BT_PBAP_TAG_ID_PBAP_FEAT_DB_ID                              0x00000004
#define BT_PBAP_TAG_ID_PBAP_FEAT_FOLDER_VER_COUNTERS                0x00000008
#define BT_PBAP_TAG_ID_PBAP_FEAT_VCARD_SELECTING                    0x00000010
#define BT_PBAP_TAG_ID_PBAP_FEAT_ENHANCED_MISSED_CALLS              0x00000020
#define BT_PBAP_TAG_ID_PBAP_FEAT_X_BT_UCI_VCARD_PROPERTY            0x00000040
#define BT_PBAP_TAG_ID_PBAP_FEAT_X_BT_UID_VCARD_PROPERTY            0x00000080
#define BT_PBAP_TAG_ID_PBAP_FEAT_CONTACT_REFERENCING                0x00000100
#define BT_PBAP_TAG_ID_PBAP_FEAT_DEFAULT_CONTACT_IMAGE_FORMAT       0x00000200

#define BT_PBAP_TARGET_LEN                 0x13
#define BT_PBAP_PHONE_BOOK_TYPE_LEN        0x12
#define BT_PBAP_VCARD_LISTING_TYPE_LEN     0x16
#define BT_PBAP_VCARD_TYPE_LEN             0x0E
#define BT_PBAP_APP_PARAM_HEAD_LEN         0x02

#define BT_PBAP_TAG_SEARCH_ATTR_LEN         0x03
#define BT_PBAP_TAG_LIST_SATRT_OFFSET_LEN   0x04
#define BT_PBAP_TAG_MAX_LIST_COUNT_LEN      0x04

#define BT_PBAP_PATH_SIM1_LEN           0x0A
#define BT_PBAP_PATH_TELECOM_LEN        0x10
#define BT_PBAP_PATH_PB_LEN             0x06
#define BT_PBAP_PATH_CALL_HISTORY_LEN   0x08

#define BT_PBAP_NAME_TELECOM_LEN         0x10
#define BT_PBAP_NAME_SIM1_TELECOM_LEN    0x1A
#define BT_PBAP_NAME_PHONE_BOOK_LEN      0x0E
#define BT_PBAP_NAME_CALL_HISTORY_LEN    0x10

typedef enum t_bt_pbap_state
{
    BT_PBAP_STATE_DISCONNECTED        = 0x00,
    BT_PBAP_STATE_CONNECTING          = 0x01,
    BT_PBAP_STATE_CONNECTED           = 0x02,
    BT_PBAP_STATE_ABORTING            = 0x03,
    BT_PBAP_STATE_GET_VCARD_LISTING   = 0x04,
    BT_PBAP_STATE_GET_VCARD_ENTRY     = 0x05,
    BT_PBAP_STATE_GET_PHONE_BOOK      = 0x06,
    BT_PBAP_STATE_GET_PHONE_BOOK_SIZE = 0x07,
} T_BT_PBAP_STATE;

typedef enum t_bt_pbap_opcode
{
    BT_PBAP_OP_SET_PHONE_BOOK     = 0x00,
    BT_PBAP_OP_PULL_PHONE_BOOK    = 0x01,
    BT_PBAP_OP_PULL_VCARD_LISTING = 0x02,
    BT_PBAP_OP_PULL_VCARD_ENTRY   = 0x03,
    BT_PBAP_OP_PULL_CONTINUE      = 0x04,
    BT_PBAP_OP_PULL_ABORT         = 0x05,
} T_BT_PBAP_OPCODE;

typedef enum t_bt_pbap_tag_id_format_value
{
    BT_PBAP_TAG_ID_FORMAT_VALUE_21 = 0x00,
    BT_PBAP_TAG_ID_FORMAT_VALUE_30 = 0x01,
} T_BT_PBAP_TAG_ID_FORMAT_VALUE;

typedef enum t_bt_pbap_tag_id_order_value
{
    BT_PBAP_TAG_ID_ORDER_VALUE_INDEXED      = 0x00,
    BT_PBAP_TAG_ID_ORDER_VALUE_ALPHANUMERIC = 0x01,
    BT_PBAP_TAG_ID_ORDER_VALUE_PHONETIC     = 0x02,
} T_BT_PBAP_TAG_ID_ORDER_VALUE;

typedef enum t_bt_pbap_tag_id_search_prop_value
{
    BT_PBAP_TAG_ID_SEARCH_PROP_VALUE_NAME   = 0x00,
    BT_PBAP_TAG_ID_SEARCH_PROP_VALUE_NUMBER = 0x01,
    BT_PBAP_TAG_ID_SEARCH_PROP_VALUE_SOUND  = 0x02,
} T_BT_PBAP_TAG_ID_SEARCH_PROP_VALUE;

typedef struct t_bt_pbap_pull_phone_book_param
{
    uint64_t                      filter;
    T_BT_PBAP_TAG_ID_FORMAT_VALUE format;
    uint16_t                      max_list_count;
    uint16_t                      start_offset;
} T_BT_PBAP_PULL_PHONE_BOOK_PARAM;

typedef struct t_bt_pbap_req_pull_phone_book
{
    T_BT_PBAP_REPOSITORY            repo;
    T_BT_PBAP_PHONE_BOOK            phone_book;
    T_BT_PBAP_PULL_PHONE_BOOK_PARAM param;
} T_BT_PBAP_REQ_PULL_PHONE_BOOK;

typedef struct t_bt_pbap_req_pull_vcard_listing
{
    uint8_t                            *folder;
    uint16_t                            folder_len;
    T_BT_PBAP_TAG_ID_ORDER_VALUE        order;
    uint8_t                            *search_value;
    uint8_t                             value_len;
    T_BT_PBAP_TAG_ID_SEARCH_PROP_VALUE  search_attr;
    uint16_t                            max_list_count;
    uint16_t                            start_offset;
} T_BT_PBAP_REQ_PULL_VCARD_LISTING;

typedef struct t_bt_pbap_req_pull_vcard_entry
{
    uint8_t                       *name;
    uint8_t                        name_len;
    uint64_t                       filter;
    T_BT_PBAP_TAG_ID_FORMAT_VALUE  format;
} T_BT_PBAP_REQ_PULL_VCARD_ENTRY;

typedef struct t_bt_pbap_cmd
{
    struct T_BT_PBAP_CMD *next;
    uint8_t               opcode;
    union
    {
        T_BT_PBAP_PATH                   pbap_path;
        T_BT_PBAP_REQ_PULL_PHONE_BOOK    pull_phone_book;
        T_BT_PBAP_REQ_PULL_VCARD_LISTING pull_vcard_listing;
        T_BT_PBAP_REQ_PULL_VCARD_ENTRY   pull_vcard_entry;
    } cmd_param;
} T_BT_PBAP_CMD;

typedef struct t_bt_pbap_get_vcard_listing_msg_data
{
    uint8_t  *data;
    uint16_t  data_len;
    uint16_t  pb_size;
    uint8_t   new_missed_calls;
    bool      data_end;
} T_BT_PBAP_GET_VCARD_LISTING_MSG_DATA;

typedef struct t_bt_pbap_link
{
    struct t_bt_pbap_link *next;
    uint8_t                bd_addr[6];
    uint8_t                state;
    uint8_t                cmd_credits;
    T_OS_QUEUE             cmd_queue;
    T_OBEX_HANDLE          handle;
    bool                   feat_flag;
    bool                   obex_over_l2c;
    uint8_t                path;
    T_BT_PBAP_REPOSITORY   repos;
    T_BT_PBAP_PATH         last_path;
    uint8_t                srm_status;
} T_BT_PBAP_LINK;

typedef struct t_bt_pbap
{
    T_OS_QUEUE link_list;
    uint8_t    conn_tout;
    uint8_t    obex_over_rfc_idx;
    uint8_t    obex_over_l2c_idx;
} T_BT_PBAP;

typedef struct t_bt_pbap_acpt_data
{
    uint8_t remote_bd[6];
    uint8_t accept_flag;
} T_BT_PBAP_ACPT_DATA;

static T_BT_PBAP *bt_pbap;

const uint8_t bt_pbap_target[BT_PBAP_TARGET_LEN + 1] =
{
    BT_PBAP_TARGET_LEN, /* indicate total length */
    OBEX_HI_TARGET,
    (uint8_t)(BT_PBAP_TARGET_LEN >> 8),
    (uint8_t)BT_PBAP_TARGET_LEN,
    /* target uuid */
    0x79, 0x61, 0x35, 0xf0, 0xf0, 0xc5, 0x11, 0xd8, 0x09, 0x66, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

const uint8_t bt_pbap_target_n_feature[BT_PBAP_TARGET_LEN + 9 + 1] =
{
    (BT_PBAP_TARGET_LEN + 9), /* indicate total length */
    OBEX_HI_TARGET,
    (uint8_t)(BT_PBAP_TARGET_LEN >> 8),
    (uint8_t)(BT_PBAP_TARGET_LEN),
    /* target uuid */
    0x79, 0x61, 0x35, 0xf0, 0xf0, 0xc5, 0x11, 0xd8, 0x09, 0x66, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66,
    /* application parameter with tag */
    OBEX_HI_APP_PARAMETER,
    /* length : 0x09 */
    0x00,
    0x09,
    BT_PBAP_TAG_ID_PBAP_FEATURE,
    0x04,
    0x00,
    0x00,
    0x00,
    0x83  /* DOWNLOAD,BROWSING, X-BT-UID VCARD */
};

/* NULL terminated ASCII : x-bt/phonebook */
const uint8_t phone_book_type[BT_PBAP_PHONE_BOOK_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(BT_PBAP_PHONE_BOOK_TYPE_LEN >> 8),
    (uint8_t)BT_PBAP_PHONE_BOOK_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x70, 0x68, 0x6f, 0x6e, 0x65, 0x62, 0x6f, 0x6f, 0x6b, 0x00
};

/* NULL terminated ASCII : x-bt/vcard */
const uint8_t vcard_type[BT_PBAP_VCARD_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(BT_PBAP_VCARD_TYPE_LEN >> 8),
    (uint8_t)BT_PBAP_VCARD_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x76, 0x63, 0x61, 0x72, 0x64, 0x00
};

/* NULL terminated ASCII : x-bt/vcard-listing */
const uint8_t vcard_listing_type[BT_PBAP_VCARD_LISTING_TYPE_LEN] =
{
    OBEX_HI_TYPE,
    (uint8_t)(BT_PBAP_VCARD_LISTING_TYPE_LEN >> 8),
    (uint8_t)BT_PBAP_VCARD_LISTING_TYPE_LEN,
    0x78, 0x2d, 0x62, 0x74, 0x2f, 0x76, 0x63, 0x61, 0x72, 0x64, 0x2d, 0x6c, 0x69, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00
};

/* NULL terminated UNICODE : telecom */
const uint8_t path_telecom[BT_PBAP_PATH_TELECOM_LEN] =
{
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6c, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x00
};

/* NULL terminated UNICODE : SIM1 */
const uint8_t path_sim1[BT_PBAP_PATH_SIM1_LEN] =
{
    0x00, 0x53, 0x00, 0x49, 0x00, 0x4d, 0x00, 0x31, 0x00, 0x00
};

/* NULL terminated UNICODE : pb */
const uint8_t path_pb[BT_PBAP_PATH_PB_LEN] =
{
    0x00, 0x70, 0x00, 0x62, 0x00, 0x00
};

/* NULL terminated UNICODE : ich */
const uint8_t path_ich[BT_PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x69, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

/* NULL terminated UNICODE : och */
const uint8_t path_och[BT_PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x6f, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

/* NULL terminated UNICODE : mch */
const uint8_t path_mch[BT_PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x6d, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

/* NULL terminated UNICODE : cch */
const uint8_t path_cch[BT_PBAP_PATH_CALL_HISTORY_LEN] =
{
    0x00, 0x63, 0x00, 0x63, 0x00, 0x68, 0x00, 0x00
};

/* UNICODE : telecom/ */
const uint8_t name_telecom[BT_PBAP_NAME_TELECOM_LEN] =
{
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x2F
};

/* UNICODE : SIM1/telecom/ */
const uint8_t name_sim1_telecom[BT_PBAP_NAME_SIM1_TELECOM_LEN] =
{
    0x00, 0x53, 0x00, 0x49, 0x00, 0x4D, 0x00, 0x31, 0x00, 0x2F,
    0x00, 0x74, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x2F
};

/* NULL terminated UNICODE : pb.vcf */
const uint8_t name_pb[BT_PBAP_NAME_PHONE_BOOK_LEN] =
{
    0x00, 0x70, 0x00, 0x62, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

/* NULL terminated UNICODE : ich.vcf */
const uint8_t name_ich[BT_PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x69, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

/* NULL terminated UNICODE : och.vcf */
const uint8_t name_och[BT_PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x6F, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

/* NULL terminated UNICODE : mch.vcf */
const uint8_t name_mch[BT_PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x6D, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

/* NULL terminated UNICODE : cch.vcf */
const uint8_t name_cch[BT_PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x63, 0x00, 0x63, 0x00, 0x68, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

/* NULL terminated UNICODE : spd.vcf */
const uint8_t name_spd[BT_PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x73, 0x00, 0x70, 0x00, 0x64, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

/* NULL terminated UNICODE : fav.vcf */
const uint8_t name_fav[BT_PBAP_NAME_CALL_HISTORY_LEN] =
{
    0x00, 0x66, 0x00, 0x61, 0x00, 0x76, 0x00, 0x2E, 0x00, 0x76, 0x00, 0x63, 0x00, 0x66, 0x00, 0x00
};

T_BT_PBAP_LINK *bt_pbap_alloc_link(uint8_t bd_addr[6])
{
    T_BT_PBAP_LINK *link;

    link = calloc(1, sizeof(T_BT_PBAP_LINK));
    if (link != NULL)
    {
        memcpy(link->bd_addr, bd_addr, 6);
        link->state = BT_PBAP_STATE_DISCONNECTED;
        os_queue_in(&bt_pbap->link_list, link);
    }

    return link;
}

void bt_pbap_free_link(T_BT_PBAP_LINK *link)
{
    if (os_queue_delete(&bt_pbap->link_list, link) == true)
    {
        T_BT_PBAP_CMD *cmd_item;

        cmd_item = os_queue_out(&link->cmd_queue);
        while (cmd_item)
        {
            if (cmd_item->opcode == BT_PBAP_OP_PULL_VCARD_ENTRY)
            {
                if (cmd_item->cmd_param.pull_vcard_entry.name != NULL)
                {
                    free(cmd_item->cmd_param.pull_vcard_entry.name);
                }
            }
            else if (cmd_item->opcode == BT_PBAP_OP_PULL_VCARD_LISTING)
            {
                if (cmd_item->cmd_param.pull_vcard_listing.folder != NULL)
                {
                    free(cmd_item->cmd_param.pull_vcard_listing.folder);
                }

                if (cmd_item->cmd_param.pull_vcard_listing.search_value != NULL)
                {
                    free(cmd_item->cmd_param.pull_vcard_listing.search_value);
                }
            }

            free(cmd_item);
            cmd_item = os_queue_out(&link->cmd_queue);
        }

        free(link);
    }
}

T_BT_PBAP_LINK *bt_pbap_find_link_by_addr(uint8_t bd_addr[6])
{
    T_BT_PBAP_LINK *link;

    link = os_queue_peek(&bt_pbap->link_list, 0);
    while (link != NULL)
    {
        if (!memcmp(link->bd_addr, bd_addr, 6))
        {
            break;
        }

        link = link->next;
    }

    return link;
}

bool bt_pbap_send_cmd_set_phone_book(T_BT_PBAP_LINK *link,
                                     T_BT_PBAP_PATH  path)
{
    const uint8_t *path_ptr;
    uint16_t       path_len;

    switch (path)
    {
    case BT_PBAP_PATH_ROOT:
        path_len = 0;
        path_ptr = NULL;
        break;

    case BT_PBAP_PATH_TELECOM:
        path_len = BT_PBAP_PATH_TELECOM_LEN;
        path_ptr = path_telecom;
        break;

    case BT_PBAP_PATH_SIM1:
        path_len = BT_PBAP_PATH_SIM1_LEN;
        path_ptr = path_sim1;
        break;

    case BT_PBAP_PATH_PB:
        path_len = BT_PBAP_PATH_PB_LEN;
        path_ptr = path_pb;
        break;

    case BT_PBAP_PATH_ICH:
        path_len = BT_PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_ich;
        break;

    case BT_PBAP_PATH_OCH:
        path_len = BT_PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_och;
        break;

    case BT_PBAP_PATH_MCH:
        path_len = BT_PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_mch;
        break;

    case BT_PBAP_PATH_CCH:
        path_len = BT_PBAP_PATH_CALL_HISTORY_LEN;
        path_ptr = path_cch;
        break;

    default:
        path_ptr = NULL;
        path_len = 0;
        break;
    }

    return obex_set_path(link->handle, 0x02, path_ptr, path_len);
}

bool bt_pbap_send_cmd_pull_phone_book(T_BT_PBAP_LINK                  *link,
                                      T_BT_PBAP_REPOSITORY             repo,
                                      T_BT_PBAP_PHONE_BOOK             phone_book,
                                      T_BT_PBAP_PULL_PHONE_BOOK_PARAM *param)
{
    uint8_t  *data;
    uint8_t   name_len;
    uint16_t  pkt_len;
    uint16_t  param_len;

    PROFILE_PRINT_INFO3("bt_pbap_send_cmd_pull_phone_book: bd_addr %s, repo %d, phone_book %d",
                        TRACE_BDADDR(link->bd_addr), repo, phone_book);

    if (repo == BT_PBAP_REPOSITORY_LOCAL)
    {
        name_len = BT_PBAP_NAME_TELECOM_LEN;
    }
    else
    {
        name_len = BT_PBAP_NAME_SIM1_TELECOM_LEN;
    }

    if (phone_book == BT_PBAP_PHONE_BOOK_PB)
    {
        name_len += BT_PBAP_NAME_PHONE_BOOK_LEN;
    }
    else
    {
        name_len += BT_PBAP_NAME_CALL_HISTORY_LEN;
    }

    param_len = OBEX_HDR_TYPE_LEN + BT_PBAP_APP_PARAM_HEAD_LEN * 4 +
                BT_PBAP_TAG_ID_FILTER_LEN + BT_PBAP_TAG_ID_FORMAT_LEN +
                BT_PBAP_TAG_ID_MAX_LIST_COUNT_LEN + BT_PBAP_TAG_ID_START_OFFSET_LEN;
    pkt_len = OBEX_HDR_NAME_LEN + name_len + BT_PBAP_PHONE_BOOK_TYPE_LEN + param_len;

    if (link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    data = malloc(pkt_len);
    if (data != NULL)
    {
        uint8_t *p;
        bool     ret;

        p = data;

        if (link->obex_over_l2c)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
        BE_UINT16_TO_STREAM(p, OBEX_HDR_NAME_LEN + name_len);
        if (repo == BT_PBAP_REPOSITORY_LOCAL)
        {
            ARRAY_TO_STREAM(p, name_telecom, BT_PBAP_NAME_TELECOM_LEN);
        }
        else
        {
            ARRAY_TO_STREAM(p, name_sim1_telecom, BT_PBAP_NAME_SIM1_TELECOM_LEN);
        }

        switch (phone_book)
        {
        case BT_PBAP_PHONE_BOOK_PB:
            ARRAY_TO_STREAM(p, name_pb, BT_PBAP_NAME_PHONE_BOOK_LEN);
            break;

        case BT_PBAP_PHONE_BOOK_ICH:
            ARRAY_TO_STREAM(p, name_ich, BT_PBAP_NAME_CALL_HISTORY_LEN);
            break;

        case BT_PBAP_PHONE_BOOK_OCH:
            ARRAY_TO_STREAM(p, name_och, BT_PBAP_NAME_CALL_HISTORY_LEN);
            break;

        case BT_PBAP_PHONE_BOOK_MCH:
            ARRAY_TO_STREAM(p, name_mch, BT_PBAP_NAME_CALL_HISTORY_LEN);
            break;

        case BT_PBAP_PHONE_BOOK_CCH:
            ARRAY_TO_STREAM(p, name_cch, BT_PBAP_NAME_CALL_HISTORY_LEN);
            break;

        case BT_PBAP_PHONE_BOOK_SPD:
            ARRAY_TO_STREAM(p, name_spd, BT_PBAP_NAME_CALL_HISTORY_LEN);
            break;

        case BT_PBAP_PHONE_BOOK_FAV:
            ARRAY_TO_STREAM(p, name_fav, BT_PBAP_NAME_CALL_HISTORY_LEN);
            break;
        }

        ARRAY_TO_STREAM(p, phone_book_type, BT_PBAP_PHONE_BOOK_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, param_len);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FILTER);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FILTER_LEN);
        BE_UINT64_TO_STREAM(p, param->filter);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FORMAT);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FORMAT_LEN);
        BE_UINT8_TO_STREAM(p, param->format);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_MAX_LIST_COUNT);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_MAX_LIST_COUNT_LEN);
        BE_UINT16_TO_STREAM(p, param->max_list_count);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_START_OFFSET);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_START_OFFSET_LEN);
        BE_UINT16_TO_STREAM(p, param->start_offset);

        if (param->max_list_count == 0)
        {
            link->state = BT_PBAP_STATE_GET_PHONE_BOOK_SIZE;
        }
        else
        {
            link->state = BT_PBAP_STATE_GET_PHONE_BOOK;
        }
        ret = obex_get_object(link->handle, data, p - data);
        free(data);

        return ret;
    }

    return false;
}

bool bt_pbap_send_cmd_pull_vcard_listing(T_BT_PBAP_LINK                   *link,
                                         T_BT_PBAP_REQ_PULL_VCARD_LISTING *param)
{
    uint8_t  *data;
    uint16_t  param_len;
    uint16_t  pkt_len;

    param_len = OBEX_HDR_TYPE_LEN + BT_PBAP_APP_PARAM_HEAD_LEN * 5 + BT_PBAP_TAG_ID_ORDER_LEN +
                param->value_len + BT_PBAP_TAG_ID_SEARCH_PROP_LEN +
                BT_PBAP_TAG_ID_MAX_LIST_COUNT_LEN + BT_PBAP_TAG_ID_START_OFFSET_LEN;
    pkt_len = OBEX_HDR_NAME_LEN + param->folder_len + BT_PBAP_VCARD_LISTING_TYPE_LEN + param_len;

    if (link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    data = malloc(pkt_len);
    if (data != NULL)
    {
        uint8_t *p;
        bool     ret;

        p = data;

        if (link->obex_over_l2c)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
        BE_UINT16_TO_STREAM(p, OBEX_HDR_NAME_LEN + param->folder_len);
        ARRAY_TO_STREAM(p, param->folder, param->folder_len);

        ARRAY_TO_STREAM(p, vcard_listing_type, BT_PBAP_VCARD_LISTING_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, param_len);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_ORDER);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_ORDER_LEN);
        BE_UINT8_TO_STREAM(p, param->order);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_SEARCH_VALUE);
        BE_UINT8_TO_STREAM(p, param->value_len);
        ARRAY_TO_STREAM(p, param->search_value, param->value_len);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_SEARCH_PROP);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_SEARCH_PROP_LEN);
        BE_UINT8_TO_STREAM(p, param->search_attr);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_MAX_LIST_COUNT);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_MAX_LIST_COUNT_LEN);
        BE_UINT16_TO_STREAM(p, param->max_list_count);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_START_OFFSET);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_START_OFFSET_LEN);
        BE_UINT16_TO_STREAM(p, param->start_offset);

        link->state = BT_PBAP_STATE_GET_VCARD_LISTING;

        ret = obex_get_object(link->handle, data, p - data);
        free(data);

        return ret;
    }

    return false;
}

bool bt_pbap_send_cmd_pull_vcard_entry(T_BT_PBAP_LINK                 *link,
                                       T_BT_PBAP_REQ_PULL_VCARD_ENTRY *param)
{
    uint8_t  *data;
    uint16_t  param_len;
    uint16_t  pkt_len;

    PROFILE_PRINT_INFO4("bt_pbap_send_cmd_pull_vcard_entry: bd_addr %s, name %s, filter 0x%016lx, format %d",
                        TRACE_BDADDR(link->bd_addr), TRACE_STRING(param->name), param->filter,
                        param->format);

    param_len = OBEX_HDR_TYPE_LEN + BT_PBAP_APP_PARAM_HEAD_LEN * 2 +
                BT_PBAP_TAG_ID_FILTER_LEN + BT_PBAP_TAG_ID_FORMAT_LEN;
    pkt_len = (OBEX_HDR_NAME_LEN + param->name_len) + BT_PBAP_VCARD_TYPE_LEN + param_len;
    if (link->obex_over_l2c)
    {
        pkt_len += OBEX_HDR_SRM_LEN;
    }

    data = malloc(pkt_len);
    if (data != NULL)
    {
        uint8_t *p;
        bool     ret;

        p = data;

        if (link->obex_over_l2c)
        {
            BE_UINT8_TO_STREAM(p, OBEX_HI_SRM);
            BE_UINT8_TO_STREAM(p, 0x01);
        }

        BE_UINT8_TO_STREAM(p, OBEX_HI_NAME);
        BE_UINT16_TO_STREAM(p, OBEX_HDR_NAME_LEN + param->name_len);
        ARRAY_TO_STREAM(p, param->name, param->name_len);

        ARRAY_TO_STREAM(p, vcard_type, BT_PBAP_VCARD_TYPE_LEN);

        BE_UINT8_TO_STREAM(p, OBEX_HI_APP_PARAMETER);
        BE_UINT16_TO_STREAM(p, param_len);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FILTER);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FILTER_LEN);
        BE_UINT64_TO_STREAM(p, param->filter);

        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FORMAT);
        BE_UINT8_TO_STREAM(p, BT_PBAP_TAG_ID_FORMAT_LEN);
        BE_UINT8_TO_STREAM(p, param->format);

        link->state = BT_PBAP_STATE_GET_VCARD_ENTRY;
        ret = obex_get_object(link->handle, data, p - data);
        free(data);

        return ret;
    }

    return false;
}

void bt_pbap_cmd_process(T_BT_PBAP_LINK *link)
{
    T_BT_PBAP_CMD *cmd;
    int32_t        ret = 0;

    if (link->cmd_credits == 0)
    {
        ret = 1;
        goto fail_no_credits;
    }

    cmd = os_queue_out(&link->cmd_queue);
    if (cmd != NULL)
    {
        bool status;

        PROFILE_PRINT_INFO1("bt_pbap_cmd_process: opcode 0x%02x", cmd->opcode);

        switch (cmd->opcode)
        {
        case BT_PBAP_OP_SET_PHONE_BOOK:
            if (link->state == BT_PBAP_STATE_CONNECTED)
            {
                status = bt_pbap_send_cmd_set_phone_book(link, cmd->cmd_param.pbap_path);
                if (status == true)
                {
                    link->last_path = cmd->cmd_param.pbap_path;
                }
            }
            else
            {
                ret = 2;
                goto fail_invalid_state;
            }
            break;

        case BT_PBAP_OP_PULL_PHONE_BOOK:
            if (link->state == BT_PBAP_STATE_CONNECTED)
            {
                status = bt_pbap_send_cmd_pull_phone_book(link,
                                                          cmd->cmd_param.pull_phone_book.repo,
                                                          cmd->cmd_param.pull_phone_book.phone_book,
                                                          &cmd->cmd_param.pull_phone_book.param);
            }
            else
            {
                ret = 2;
                goto fail_invalid_state;
            }
            break;

        case BT_PBAP_OP_PULL_VCARD_LISTING:
            if (link->state == BT_PBAP_STATE_CONNECTED)
            {
                status = bt_pbap_send_cmd_pull_vcard_listing(link, &cmd->cmd_param.pull_vcard_listing);
                if (cmd->cmd_param.pull_vcard_listing.folder)
                {
                    free(cmd->cmd_param.pull_vcard_listing.folder);
                    cmd->cmd_param.pull_vcard_listing.folder = NULL;
                }
                if (cmd->cmd_param.pull_vcard_listing.search_value)
                {
                    free(cmd->cmd_param.pull_vcard_listing.search_value);
                    cmd->cmd_param.pull_vcard_listing.search_value = NULL;
                }
            }
            else
            {
                ret = 2;
                goto fail_invalid_state;
            }
            break;

        case BT_PBAP_OP_PULL_VCARD_ENTRY:
            if (link->state == BT_PBAP_STATE_CONNECTED)
            {
                status = bt_pbap_send_cmd_pull_vcard_entry(link, &cmd->cmd_param.pull_vcard_entry);
                if (cmd->cmd_param.pull_vcard_entry.name != NULL)
                {
                    free(cmd->cmd_param.pull_vcard_entry.name);
                    cmd->cmd_param.pull_vcard_entry.name = NULL;
                }
            }
            else
            {
                ret = 2;
                goto fail_invalid_state;
            }
            break;

        case BT_PBAP_OP_PULL_CONTINUE:
            {
                status = obex_get_object_continue(link->handle);
            }
            break;

        case BT_PBAP_OP_PULL_ABORT:
            {
                status = obex_abort(link->handle);
                if (status == true)
                {
                    link->state = BT_PBAP_STATE_ABORTING;
                }
            }
            break;

        default:
            ret = 3;
            goto fail_unknown_opcode;
        }

        if (status == false)
        {
            ret = 4;
            goto fail_send_cmd;
        }

        free(cmd);
        link->cmd_credits = 0;
    }

    return;

fail_send_cmd:
fail_unknown_opcode:
fail_invalid_state:
    free(cmd);
fail_no_credits:
    PROFILE_PRINT_ERROR1("bt_pbap_cmd_process: failed %d", -ret);
}

void bt_pbap_obex_cback(uint8_t  msg_type,
                        void    *msg)
{
    T_BT_PBAP_LINK *link;

    PROFILE_PRINT_TRACE1("bt_pbap_obex_cback: msg_type 0x%02x", msg_type);

    switch (msg_type)
    {
    case OBEX_MSG_CONN_IND:
        break;

    case OBEX_MSG_CONN_CMPL:
        {
            T_OBEX_CONN_CMPL_DATA *data = (T_OBEX_CONN_CMPL_DATA *)msg;

            link = bt_pbap_find_link_by_addr(data->bd_addr);
            if (link != NULL)
            {
                T_BT_MSG_PAYLOAD payload;

                link->state = BT_PBAP_STATE_CONNECTED;
                os_queue_init(&link->cmd_queue);
                link->cmd_credits = 1;

                memcpy(payload.bd_addr, data->bd_addr, 6);
                payload.msg_buf = NULL;
                bt_mgr_dispatch(BT_MSG_PBAP_CONN_CMPL, &payload);
            }
            else
            {
                obex_disconn_req(data->handle);
            }
        }
        break;

    case OBEX_MSG_DISCONN:
        {
            T_OBEX_DISCONN_CMPL_DATA *data = (T_OBEX_DISCONN_CMPL_DATA *)msg;

            link = bt_pbap_find_link_by_addr(data->bd_addr);
            if (link != NULL)
            {
                T_BT_MSG_PAYLOAD payload;
                uint8_t          link_state;

                link_state = link->state;
                bt_pbap_free_link(link);
                memcpy(payload.bd_addr, data->bd_addr, 6);
                if (link_state == BT_PBAP_STATE_CONNECTING)
                {
                    payload.msg_buf = &data->cause;
                    bt_mgr_dispatch(BT_MSG_PBAP_CONN_FAIL, &payload);
                }
                else
                {
                    T_BT_PBAP_DISCONN_INFO info;
                    info.cause = data->cause;
                    payload.msg_buf = &info;
                    bt_mgr_dispatch(BT_MSG_PBAP_DISCONN_CMPL, &payload);
                }
            }
        }
        break;

    case OBEX_MSG_SET_PATH_DONE:
        {
            T_OBEX_SET_PATH_CMPL_DATA *data = (T_OBEX_SET_PATH_CMPL_DATA *)msg;

            link = bt_pbap_find_link_by_addr(data->bd_addr);
            if (link != NULL)
            {
                T_BT_PBAP_SET_PHONE_BOOK_CMPL msg_buf;
                T_BT_MSG_PAYLOAD              payload;

                if (data->cause == OBEX_RSP_SUCCESS)
                {
                    msg_buf.result = true;
                    link->path = link->last_path;
                }
                else
                {
                    msg_buf.result = false;
                }
                msg_buf.path = link->last_path;

                memcpy(payload.bd_addr, data->bd_addr, 6);
                payload.msg_buf = &msg_buf;
                bt_mgr_dispatch(BT_MSG_PBAP_SET_PHONE_BOOK_CMPL, &payload);

                link->cmd_credits = 1;
                bt_pbap_cmd_process(link);
            }
        }
        break;

    case OBEX_MSG_ABORT_DONE:
        {
            T_OBEX_ABORT_CMPL_DATA *data = (T_OBEX_ABORT_CMPL_DATA *)msg;

            link = bt_pbap_find_link_by_addr(data->bd_addr);
            if (link != NULL)
            {
                link->state = BT_PBAP_STATE_CONNECTED;
                link->cmd_credits = 1;
                link->srm_status = SRM_DISABLE;
                bt_pbap_cmd_process(link);
            }
        }
        break;

    case OBEX_MSG_GET_OBJECT:
        {
            T_OBEX_GET_OBJECT_CMPL_DATA *obj_data = (T_OBEX_GET_OBJECT_CMPL_DATA *)msg;

            link = bt_pbap_find_link_by_addr(obj_data->bd_addr);
            if (link != NULL)
            {
                T_BT_MSG_PAYLOAD  payload;
                uint8_t          *hdr_data;
                uint16_t          hdr_data_len;
                bool              ret;

                link->cmd_credits = 1;
                memcpy(payload.bd_addr, obj_data->bd_addr, 6);

                if (obj_data->rsp_code != OBEX_RSP_SUCCESS && obj_data->rsp_code != OBEX_RSP_CONTINUE)
                {
                    PROFILE_PRINT_ERROR1("bt_pbap_obex_cback: error rsp_code=0x%x", obj_data->rsp_code);
                }

                link->srm_status = obj_data->srm_status;

                if (link->state == BT_PBAP_STATE_GET_PHONE_BOOK)
                {
                    T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA data = {0};

                    ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                               obj_data->data_len,
                                               OBEX_HI_APP_PARAMETER,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret)
                    {
                        uint8_t  *tag_data;
                        uint16_t  tag_data_len;

                        ret = obex_find_value_in_app_param(hdr_data,
                                                           hdr_data_len,
                                                           BT_PBAP_TAG_ID_PB_SIZE,
                                                           &tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.pb_size, tag_data);
                        }

                        ret = obex_find_value_in_app_param(hdr_data,
                                                           hdr_data_len,
                                                           BT_PBAP_TAG_ID_NEW_MISS_CALL,
                                                           &tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.new_missed_calls, tag_data);
                        }
                    }

                    ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                               obj_data->data_len,
                                               OBEX_HI_BODY,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                                   obj_data->data_len,
                                                   OBEX_HI_END_BODY,
                                                   &hdr_data,
                                                   &hdr_data_len);
                    }

                    if (ret)
                    {
                        data.data = hdr_data;
                        data.data_len = hdr_data_len;
                    }

                    if (obj_data->rsp_code != OBEX_RSP_CONTINUE)
                    {
                        data.data_end = true;
                        link->state = BT_PBAP_STATE_CONNECTED;
                        link->srm_status = SRM_DISABLE;
                    }

                    payload.msg_buf = &data;
                    bt_mgr_dispatch(BT_MSG_PBAP_GET_PHONE_BOOK_CMPL, &payload);
                }
                else if (link->state == BT_PBAP_STATE_GET_PHONE_BOOK_SIZE)
                {
                    T_BT_PBAP_GET_PHONE_BOOK_MSG_DATA data = {0};

                    ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                               obj_data->data_len,
                                               OBEX_HI_APP_PARAMETER,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret)
                    {
                        uint8_t  *tag_data;
                        uint16_t  tag_data_len;

                        ret = obex_find_value_in_app_param(hdr_data,
                                                           hdr_data_len,
                                                           BT_PBAP_TAG_ID_PB_SIZE,
                                                           &tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.pb_size, tag_data);
                        }

                        ret = obex_find_value_in_app_param(hdr_data,
                                                           hdr_data_len,
                                                           BT_PBAP_TAG_ID_NEW_MISS_CALL,
                                                           &tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.new_missed_calls, tag_data);
                        }
                    }

                    data.data_end = true;
                    link->state = BT_PBAP_STATE_CONNECTED;

                    payload.msg_buf = &data;
                    bt_mgr_dispatch(BT_MSG_PBAP_GET_PHONE_BOOK_SIZE_CMPL, &payload);
                }
                else if (link->state == BT_PBAP_STATE_GET_VCARD_LISTING)
                {
                    T_BT_PBAP_GET_VCARD_LISTING_MSG_DATA data = {0};
                    T_BT_PBAP_CALLER_ID_NAME             caller_id_name;

                    ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                               obj_data->data_len,
                                               OBEX_HI_APP_PARAMETER,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret)
                    {
                        uint8_t  *tag_data;
                        uint16_t  tag_data_len;

                        ret = obex_find_value_in_app_param(hdr_data,
                                                           hdr_data_len,
                                                           BT_PBAP_TAG_ID_PB_SIZE,
                                                           &tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.pb_size, tag_data);
                        }

                        ret = obex_find_value_in_app_param(hdr_data,
                                                           hdr_data_len,
                                                           BT_PBAP_TAG_ID_NEW_MISS_CALL,
                                                           &tag_data,
                                                           &tag_data_len);
                        if (ret)
                        {
                            BE_STREAM_TO_UINT16(data.new_missed_calls, tag_data);
                        }
                    }

                    ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                               obj_data->data_len,
                                               OBEX_HI_BODY,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                                   obj_data->data_len,
                                                   OBEX_HI_END_BODY,
                                                   &hdr_data,
                                                   &hdr_data_len);
                    }

                    if (ret)
                    {
                        data.data = hdr_data;
                        data.data_len = hdr_data_len;
                    }

                    if (obj_data->rsp_code != OBEX_RSP_CONTINUE)
                    {
                        data.data_end = true;
                        link->state = BT_PBAP_STATE_CONNECTED;
                    }


                    if (data.data != NULL)
                    {
                        uint8_t *temp_ptr;

                        data.data[data.data_len - 1] = '\0';
                        temp_ptr = (uint8_t *)strstr((char *)data.data, "name");
                        if (temp_ptr != NULL)
                        {
                            uint8_t     step;
                            uint16_t    temp_len;

                            temp_len = data.data_len - (temp_ptr - data.data);
                            /* name="xxx" */
                            step = 0;
                            while (temp_len)
                            {
                                if (step == 0)
                                {
                                    if (*temp_ptr == 0x22)
                                    {
                                        step = 1;
                                        caller_id_name.name_len = 0;
                                        caller_id_name.name_ptr = temp_ptr + 1;
                                    }
                                }
                                else if (step == 1)
                                {
                                    if (*temp_ptr == 0x22)
                                    {
                                        break;
                                    }
                                    caller_id_name.name_len++;
                                }
                                temp_ptr++;
                                temp_len--;
                            }
                        }

                        payload.msg_buf = &caller_id_name;
                        bt_mgr_dispatch(BT_MSG_PBAP_CALLER_ID_NAME, &payload);
                    }
                    else
                    {
                        if (data.data_end == false)
                        {
                            bt_pbap_pull_continue(link->bd_addr);
                        }
                        else
                        {
                            caller_id_name.name_ptr = NULL;
                            caller_id_name.name_len = 0;
                            payload.msg_buf = &caller_id_name;
                            bt_mgr_dispatch(BT_MSG_PBAP_CALLER_ID_NAME, &payload);
                        }
                    }
                }
                else if (link->state == BT_PBAP_STATE_GET_VCARD_ENTRY)
                {
                    T_BT_PBAP_GET_VCARD_ENTRY_MSG_DATA data = {0};

                    ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                               obj_data->data_len,
                                               OBEX_HI_BODY,
                                               &hdr_data,
                                               &hdr_data_len);
                    if (ret == false)
                    {
                        ret = obex_find_hdr_in_pkt(obj_data->p_data,
                                                   obj_data->data_len,
                                                   OBEX_HI_END_BODY,
                                                   &hdr_data,
                                                   &hdr_data_len);
                    }

                    if (ret)
                    {
                        data.data = hdr_data;
                        data.data_len = hdr_data_len;
                    }
                    if (obj_data->rsp_code != OBEX_RSP_CONTINUE)
                    {
                        data.data_end = true;
                        link->state = BT_PBAP_STATE_CONNECTED;
                    }

                    payload.msg_buf = &data;
                    bt_mgr_dispatch(BT_MSG_PBAP_GET_VCARD_ENTRY_CMPL, &payload);
                }

                bt_pbap_cmd_process(link);
            }
        }
        break;

    default:
        break;
    }
}

bool bt_pbap_connect_over_rfc_req(uint8_t bd_addr[6],
                                  uint8_t server_chann,
                                  bool    feat_flag)
{
    T_BT_PBAP_LINK *link;
    uint8_t        *param;
    int32_t         ret = 0;

    /* TODO: support only one mas per link now */
    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link)
    {
        ret = 1;
        goto fail_link_exist;
    }

    link = bt_pbap_alloc_link(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    link->feat_flag = feat_flag;
    if (feat_flag)
    {
        param = (uint8_t *)bt_pbap_target_n_feature;
    }
    else
    {
        param = (uint8_t *)bt_pbap_target;
    }

    if (obex_conn_req_over_rfc(bd_addr,
                               param,
                               server_chann,
                               bt_pbap_obex_cback,
                               &link->handle) == false)
    {
        ret = 3;
        goto fail_conn_fail;
    }

    link->state = BT_PBAP_STATE_CONNECTING;
    link->obex_over_l2c = false;
    return true;

fail_conn_fail:
    bt_pbap_free_link(link);
fail_alloc_link:
fail_link_exist:
    PROFILE_PRINT_ERROR4("bt_pbap_conn_over_rfc: failed %d, bd_addr %s, server_chann 0x%02x, feat_flag %d",
                         -ret, TRACE_BDADDR(bd_addr), server_chann, feat_flag);
    return false;
}

bool bt_pbap_connect_over_l2c_req(uint8_t  bd_addr[6],
                                  uint16_t l2c_psm,
                                  bool     feat_flag)
{
    T_BT_PBAP_LINK *link;
    uint8_t        *param;
    int32_t         ret = 0;

    /* TODO: support only one mas per link now */
    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link)
    {
        ret = 1;
        goto fail_link_exist;
    }

    link = bt_pbap_alloc_link(bd_addr);
    if (link == NULL)
    {
        ret = 2;
        goto fail_alloc_link;
    }

    link->feat_flag = feat_flag;
    if (feat_flag)
    {
        param = (uint8_t *)bt_pbap_target_n_feature;
    }
    else
    {
        param = (uint8_t *)bt_pbap_target;
    }

    if (obex_conn_req_over_l2c(bd_addr,
                               param,
                               l2c_psm,
                               bt_pbap_obex_cback,
                               &link->handle) == false)
    {
        ret = 3;
        goto fail_conn_fail;

    }

    link->state = BT_PBAP_STATE_CONNECTING;
    link->obex_over_l2c = true;
    return true;

fail_conn_fail:
    bt_pbap_free_link(link);
fail_alloc_link:
fail_link_exist:
    PROFILE_PRINT_ERROR4("bt_pbap_connect_over_l2c_req: failed %d, bd_addr %s, l2c_psm 0x%04x, feat_flag %d",
                         -ret, TRACE_BDADDR(bd_addr), l2c_psm, feat_flag);
    return false;
}

bool bt_pbap_disconnect_req(uint8_t bd_addr[6])
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return obex_disconn_req(link->handle);
    }

    return false;
}

bool bt_pbap_cmd_enqueue(T_BT_PBAP_LINK *link,
                         uint8_t         opcode,
                         void           *param)
{
    T_BT_PBAP_CMD *cmd;
    int32_t        ret = 0;

    PROFILE_PRINT_INFO1("bt_pbap_cmd_enqueue: opcode 0x%02x", opcode);

    cmd = calloc(1, sizeof(T_BT_PBAP_CMD));
    if (cmd == NULL)
    {
        ret = 1;
        goto fail_alloc_cmd;
    }

    cmd->opcode = opcode;
    switch (opcode)
    {
    case BT_PBAP_OP_SET_PHONE_BOOK:
        cmd->cmd_param.pbap_path = *(T_BT_PBAP_PATH *)param;
        break;

    case BT_PBAP_OP_PULL_PHONE_BOOK:
        memcpy(&cmd->cmd_param.pull_phone_book, param, sizeof(T_BT_PBAP_REQ_PULL_PHONE_BOOK));
        break;

    case BT_PBAP_OP_PULL_VCARD_LISTING:
        {
            T_BT_PBAP_REQ_PULL_VCARD_LISTING *tmp = (T_BT_PBAP_REQ_PULL_VCARD_LISTING *)param;

            if (tmp->folder_len > 0)
            {
                cmd->cmd_param.pull_vcard_listing.folder = malloc(tmp->folder_len);
                if (cmd->cmd_param.pull_vcard_listing.folder == NULL)
                {
                    ret = 2;
                    goto fail_build_cmd;
                }

                memcpy(cmd->cmd_param.pull_vcard_listing.folder, tmp->folder, tmp->folder_len);
            }

            cmd->cmd_param.pull_vcard_listing.search_value = malloc(tmp->value_len);
            if (cmd->cmd_param.pull_vcard_listing.search_value == NULL)
            {
                if (cmd->cmd_param.pull_vcard_listing.folder != NULL)
                {
                    free(cmd->cmd_param.pull_vcard_listing.folder);
                }
                ret = 3;
                goto fail_build_cmd;
            }

            cmd->cmd_param.pull_vcard_listing.folder_len = tmp->folder_len;
            cmd->cmd_param.pull_vcard_listing.order = tmp->order;
            memcpy(cmd->cmd_param.pull_vcard_listing.search_value, tmp->search_value, tmp->value_len);
            cmd->cmd_param.pull_vcard_listing.value_len = tmp->value_len;
            cmd->cmd_param.pull_vcard_listing.search_attr = tmp->search_attr;
            cmd->cmd_param.pull_vcard_listing.max_list_count = tmp->max_list_count;
            cmd->cmd_param.pull_vcard_listing.start_offset = tmp->start_offset;
        }
        break;

    case BT_PBAP_OP_PULL_VCARD_ENTRY:
        {
            T_BT_PBAP_REQ_PULL_VCARD_ENTRY *tmp = (T_BT_PBAP_REQ_PULL_VCARD_ENTRY *)param;

            cmd->cmd_param.pull_vcard_entry.name = malloc(tmp->name_len);
            if (cmd->cmd_param.pull_vcard_entry.name == NULL)
            {
                ret = 4;
                goto fail_build_cmd;
            }

            memcpy(cmd->cmd_param.pull_vcard_entry.name, tmp->name, tmp->name_len);
            cmd->cmd_param.pull_vcard_entry.name_len = tmp->name_len;
            cmd->cmd_param.pull_vcard_entry.filter = tmp->filter;
            cmd->cmd_param.pull_vcard_entry.format = tmp->format;
        }
        break;

    case BT_PBAP_OP_PULL_CONTINUE:
        break;

    case BT_PBAP_OP_PULL_ABORT:
        break;

    default:
        ret = 5;
        goto fail_build_cmd;
    }

    os_queue_in(&link->cmd_queue, cmd);
    bt_pbap_cmd_process(link);

    return true;

fail_build_cmd:
    free(cmd);
fail_alloc_cmd:
    PROFILE_PRINT_ERROR1("bt_pbap_cmd_enqueue: failed %d", -ret);
    return false;
}

bool bt_pbap_phone_book_pull(uint8_t              bd_addr[6],
                             T_BT_PBAP_REPOSITORY repos,
                             T_BT_PBAP_PHONE_BOOK phone_book,
                             uint16_t             start_offset,
                             uint16_t             max_list_count,
                             uint64_t             filter)
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_PBAP_REQ_PULL_PHONE_BOOK   tmp;
        T_BT_PBAP_PULL_PHONE_BOOK_PARAM param;

        param.filter = filter;
        param.format = BT_PBAP_TAG_ID_FORMAT_VALUE_21;
        param.max_list_count = max_list_count;
        param.start_offset = start_offset;

        tmp.repo = repos;
        tmp.phone_book = phone_book;
        tmp.param = param;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_pbap_cmd_enqueue(link, BT_PBAP_OP_PULL_PHONE_BOOK, &tmp);
    }

    return false;
}

bool bt_pbap_phone_book_size_get(uint8_t              bd_addr[6],
                                 T_BT_PBAP_REPOSITORY repos,
                                 T_BT_PBAP_PHONE_BOOK phone_book)
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_PBAP_REQ_PULL_PHONE_BOOK   tmp;
        T_BT_PBAP_PULL_PHONE_BOOK_PARAM param;

        param.filter = 0;
        param.format = BT_PBAP_TAG_ID_FORMAT_VALUE_21;
        param.max_list_count = 0;
        param.start_offset = 0;

        tmp.repo = repos;
        tmp.phone_book = phone_book;
        tmp.param = param;

        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_pbap_cmd_enqueue(link, BT_PBAP_OP_PULL_PHONE_BOOK, &tmp);
    }

    return false;
}

bool bt_pbap_pull_continue(uint8_t bd_addr[6])
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        if (link->srm_status == SRM_ENABLE)
        {
            return true;
        }
        bt_sniff_mode_exit(link->bd_addr, false);

        return bt_pbap_cmd_enqueue(link, BT_PBAP_OP_PULL_CONTINUE, NULL);
    }

    return false;
}

bool bt_pbap_pull_abort(uint8_t bd_addr[6])
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        bt_sniff_mode_exit(link->bd_addr, false);
        return bt_pbap_cmd_enqueue(link, BT_PBAP_OP_PULL_ABORT, NULL);
    }

    return false;
}

bool bt_pbap_phone_book_set(uint8_t              bd_addr[6],
                            T_BT_PBAP_REPOSITORY repos,
                            T_BT_PBAP_PATH       path)
{
    T_BT_PBAP_LINK *link;
    bool            ret = false;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        uint8_t pbap_path = BT_PBAP_PATH_ROOT;

        link->repos = repos;
        ret = bt_pbap_cmd_enqueue(link, BT_PBAP_OP_SET_PHONE_BOOK, &pbap_path);
        if (path != BT_PBAP_PATH_ROOT)
        {
            if (repos == BT_PBAP_REPOSITORY_SIM1)
            {
                pbap_path = BT_PBAP_PATH_SIM1;
                ret = (ret && bt_pbap_cmd_enqueue(link, BT_PBAP_OP_SET_PHONE_BOOK, &pbap_path));
            }

            pbap_path = BT_PBAP_PATH_TELECOM;
            ret = (ret && bt_pbap_cmd_enqueue(link, BT_PBAP_OP_SET_PHONE_BOOK, &pbap_path));

            if (path != BT_PBAP_PATH_TELECOM)
            {
                ret = (ret && bt_pbap_cmd_enqueue(link, BT_PBAP_OP_SET_PHONE_BOOK, &path));
            }
        }
    }

    return ret;
}

bool bt_pbap_pull_vcard_listing(uint8_t                             bd_addr[6],
                                uint8_t                            *folder,
                                uint16_t                            folder_len,
                                T_BT_PBAP_TAG_ID_ORDER_VALUE        order,
                                uint8_t                            *search_value,
                                uint8_t                             value_len,
                                T_BT_PBAP_TAG_ID_SEARCH_PROP_VALUE  search_attr,
                                uint16_t                            max_list_count,
                                uint16_t                            start_offset)
{
    T_BT_PBAP_LINK *link;
    bool            ret = false;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_PBAP_REQ_PULL_VCARD_LISTING tmp;

        tmp.folder = folder;
        tmp.folder_len = folder_len;
        tmp.order = order;
        tmp.search_value = search_value;
        tmp.value_len = value_len;
        tmp.search_attr = search_attr;
        tmp.max_list_count = max_list_count;
        tmp.start_offset = start_offset;

        ret = bt_pbap_cmd_enqueue(link, BT_PBAP_OP_PULL_VCARD_LISTING, &tmp);
    }

    return ret;
}

bool bt_pbap_vcard_listing_by_number_pull(uint8_t  bd_addr[6],
                                          char    *phone_number)
{
    return bt_pbap_pull_vcard_listing(bd_addr,
                                      NULL,
                                      0,
                                      BT_PBAP_TAG_ID_ORDER_VALUE_INDEXED,
                                      (uint8_t *)phone_number,
                                      strlen(phone_number) + 1,
                                      BT_PBAP_TAG_ID_SEARCH_PROP_VALUE_NUMBER,
                                      4,
                                      0);
}

bool bt_pbap_pull_vcard_entry(uint8_t   bd_addr[6],
                              uint8_t  *name,
                              uint8_t   name_len,
                              uint64_t  filter)
{
    T_BT_PBAP_LINK *link;
    bool            ret = false;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_BT_PBAP_REQ_PULL_VCARD_ENTRY tmp;

        tmp.name = name;
        tmp.name_len = name_len;
        tmp.filter = filter;
        tmp.format = BT_PBAP_TAG_ID_FORMAT_VALUE_21;

        ret = bt_pbap_cmd_enqueue(link, BT_PBAP_OP_PULL_VCARD_ENTRY, &tmp);
    }

    return ret;
}

bool bt_pbap_init(void)
{
    int32_t ret = 0;

    bt_pbap = calloc(1, sizeof(T_BT_PBAP));
    if (bt_pbap == NULL)
    {
        ret = 1;
        goto fail_alloc_pbap;
    }

    if (0)  /* not support set feature yet */
    {
        BE_UINT32_TO_ARRAY(&bt_pbap_target_n_feature[BT_PBAP_TARGET_LEN + 5], 0);
    }

    bt_pbap->conn_tout = 30;

    return true;

fail_alloc_pbap:
    PROFILE_PRINT_ERROR1("bt_pbap_init: failed %d", -ret);
    return false;
}

bool bt_pbap_roleswap_info_get(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *info)
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_OBEX_ROLESWAP_INFO obex_info;

        if (obex_roleswap_info_get(link->handle, &obex_info) == true)
        {
            memcpy(info->bd_addr, bd_addr, 6);
            info->feat_flag = link->feat_flag;
            info->pbap_state = link->state;
            info->path = link->path;
            info->repos = link->repos;

            info->obex_conn_id = obex_info.conn_id;
            info->l2c_cid = obex_info.cid;
            info->local_max_pkt_len = obex_info.local_max_pkt_len;
            info->remote_max_pkt_len = obex_info.remote_max_pkt_len;
            info->obex_state = (uint8_t)obex_info.state;
            info->obex_psm = obex_info.psm;

            if (info->obex_psm)
            {
                info->data_offset = obex_info.data_offset;
            }
            else
            {
                info->rfc_dlci = obex_info.rfc_dlci;
            }

            return true;
        }
    }

    return false;
}

bool bt_pbap_roleswap_info_set(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *info)
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link == NULL)
    {
        link = bt_pbap_alloc_link(bd_addr);
    }

    if (link != NULL)
    {
        T_OBEX_ROLESWAP_INFO obex_info;

        link->state = info->pbap_state;
        link->feat_flag = info->feat_flag;
        link->path = info->path;
        link->repos = info->repos;

        /* set client obex info */
        obex_info.conn_id = info->obex_conn_id;
        obex_info.cid = info->l2c_cid;
        obex_info.local_max_pkt_len = info->local_max_pkt_len;
        obex_info.remote_max_pkt_len = info->remote_max_pkt_len;
        obex_info.role = OBEX_ROLE_CLIENT;
        obex_info.state = (T_OBEX_STATE)info->obex_state;
        obex_info.psm = info->obex_psm;

        if (obex_info.psm)
        {
            obex_info.data_offset = info->data_offset;
        }
        else
        {
            obex_info.rfc_dlci = info->rfc_dlci;
        }

        if (obex_roleswap_info_set(bd_addr, bt_pbap_obex_cback, &obex_info, &link->handle) == true)
        {
            return true;
        }

        bt_pbap_free_link(link);
    }

    return false;
}

bool bt_pbap_roleswap_info_del(uint8_t bd_addr[6])
{
    T_BT_PBAP_LINK *link;

    link = bt_pbap_find_link_by_addr(bd_addr);
    if (link != NULL)
    {
        T_OBEX_HANDLE obex_handle;

        obex_handle = link->handle;
        bt_pbap_free_link(link);

        return obex_roleswap_info_del(obex_handle);
    }

    return false;
}

bool bt_pbap_service_id_get(uint8_t *service_id)
{
    if (bt_pbap != NULL)
    {
        *service_id = obex_service_id_get();
        return true;
    }

    return false;
}
#else
#include <stdint.h>
#include <stdbool.h>
#include "bt_pbap_int.h"
#include "bt_pbap.h"

bool bt_pbap_init(void)
{
    return false;
}

bool bt_pbap_connect_over_rfc_req(uint8_t bd_addr[6],
                                  uint8_t server_chann,
                                  bool    feat_flag)
{
    return false;
}

bool bt_pbap_connect_over_l2c_req(uint8_t  bd_addr[6],
                                  uint16_t l2c_psm,
                                  bool     feat_flag)
{
    return false;
}

bool bt_pbap_disconnect_req(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_phone_book_pull(uint8_t              bd_addr[6],
                             T_BT_PBAP_REPOSITORY repos,
                             T_BT_PBAP_PHONE_BOOK phone_book,
                             uint16_t             start_offset,
                             uint16_t             max_list_count,
                             uint64_t             filter)
{
    return false;
}

bool bt_pbap_phone_book_size_get(uint8_t              bd_addr[6],
                                 T_BT_PBAP_REPOSITORY repos,
                                 T_BT_PBAP_PHONE_BOOK phone_book)
{
    return false;
}

bool bt_pbap_pull_continue(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_pull_abort(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_phone_book_set(uint8_t              bd_addr[6],
                            T_BT_PBAP_REPOSITORY repos,
                            T_BT_PBAP_PATH       path)
{
    return false;
}

bool bt_pbap_vcard_listing_by_number_pull(uint8_t  bd_addr[6],
                                          char    *phone_number)
{
    return false;
}

bool bt_pbap_pull_vcard_entry(uint8_t   bd_addr[6],
                              uint8_t  *p_name,
                              uint8_t   name_len,
                              uint64_t  filter)
{
    return false;
}

bool bt_pbap_roleswap_info_get(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *info)
{
    return false;
}

bool bt_pbap_roleswap_info_set(uint8_t               bd_addr[6],
                               T_ROLESWAP_PBAP_INFO *info)
{
    return false;
}

bool bt_pbap_roleswap_info_del(uint8_t bd_addr[6])
{
    return false;
}

bool bt_pbap_service_id_get(uint8_t *service_id)
{
    return false;
}
#endif
