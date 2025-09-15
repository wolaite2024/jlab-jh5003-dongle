/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _BT_HID_PARSER_H_
#define _BT_HID_PARSER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum t_bt_hid_item_type
{
    BT_HID_ITEM_TYPE_MAIN,
    BT_HID_ITEM_TYPE_GLOBAL,
    BT_HID_ITEM_TYPE_LOCAL,
    BT_HID_ITEM_TYPE_RESERVED
} T_BT_HID_ITEM_TYPE;

typedef enum t_bt_hid_main_item_tag
{
    BT_HID_MAIN_ITEM_TAG_INPUT    = 0x08,
    BT_HID_MAIN_ITEM_TAG_OUTPUT   = 0x09,
    BT_HID_MAIN_ITEM_TAG_COLL     = 0x0a,
    BT_HID_MAIN_ITEM_TAG_FEATURE  = 0x0b,
    BT_HID_MAIN_ITEM_TAG_ENDCOLL  = 0x0c
} T_BT_HID_MAIN_ITEM_TAG;

typedef enum t_bt_hid_global_item_tag
{
    BT_HID_GLOBAL_ITEM_TAG_USAGE_PAGE,
    BT_HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUN,
    BT_HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM,
    BT_HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM,
    BT_HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUN,
    BT_HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT,
    BT_HID_GLOBAL_ITEM_TAG_UNIT,
    BT_HID_GLOBAL_ITEM_TAG_REPORT_SIZE,
    BT_HID_GLOBAL_ITEM_TAG_REPORT_ID,
    BT_HID_GLOBAL_ITEM_TAG_REPORT_COUNT,
    BT_HID_GLOBAL_ITEM_TAG_PUSH,
    BT_HID_GLOBAL_ITEM_TAG_POP
} T_BT_HID_GLOBAL_ITEM_TAG;

typedef enum t_bt_hid_loacl_item_tag
{
    BT_HID_LOCAL_ITEM_TAG_USAGE,
    BT_HID_LOCAL_ITEM_TAG_USAGE_MINIMUM,
    BT_HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM,
    BT_HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX,
    BT_HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM,
    BT_HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM,
    BT_HID_LOCAL_ITEM_TAG_STRING_INDEX,
    BT_HID_LOCAL_ITEM_TAG_STRING_MINIMUM,
    BT_HID_LOCAL_ITEM_TAG_STRING_MAXIMUM,
    BT_HID_LOCAL_ITEM_TAG_DELIMITER
} T_BT_HID_LOCAL_ITEM_TAG;

typedef struct t_bt_hid_descriptor_item
{
    int32_t  item_value;
    uint16_t item_size;
    uint8_t  item_type;
    uint8_t  item_tag;
    uint8_t  data_size;
} T_BT_HID_DESCRIPTOR_ITEM;

typedef enum t_bt_hid_parser_state
{
    BT_HID_PARSER_STATE_SCAN_FOR_REPORT_ITEM,
    BT_HID_PARSER_STATE_USAGES_AVAILABLE,
    BT_HID_PARSER_STATE_COMPLETE,
} T_BT_HID_PARSER_STATE;

typedef enum t_bt_hid_report_type
{
    BT_HID_REPORT_TYPE_RESERVED,
    BT_HID_REPORT_TYPE_INPUT,
    BT_HID_REPORT_TYPE_OUTPUT,
    BT_HID_REPORT_TYPE_FEATURE
} T_BT_HID_REPORT_TYPE;

typedef enum t_bt_hid_report_id_status
{
    BT_HID_REPORT_ID_UNDECLARED,
    BT_HID_REPORT_ID_VALID,
    BT_HID_REPORT_ID_INVALID
} T_BT_HID_REPORT_ID_STATUS;

typedef struct t_bt_hid_parser
{
    T_BT_HID_PARSER_STATE     state;

    const uint8_t            *descriptor;
    uint16_t                  descriptor_len;
    uint16_t                  descriptor_pos;
    T_BT_HID_DESCRIPTOR_ITEM  descriptor_item;

    const uint8_t            *report;
    uint16_t                  report_len;
    T_BT_HID_REPORT_TYPE      report_type;
    uint16_t                  report_pos_in_bit;

    uint16_t                  usage_pos;
    uint16_t                  usage_page;

    uint32_t                  usage_minimum;
    uint32_t                  usage_maximum;
    uint16_t                  available_usages;
    uint8_t                   required_usages;
    uint8_t                   active_record;
    uint8_t                   have_usage_min;
    uint8_t                   have_usage_max;

    int32_t                   global_logical_minimum;
    int32_t                   global_logical_maximum;
    uint16_t                  global_usage_page;
    uint8_t                   global_report_size;
    uint8_t                   global_report_count;
    uint8_t                   global_report_id;
} T_BT_HID_PARSER;

void bt_hid_parser_init(T_BT_HID_PARSER      *parser,
                        const uint8_t        *hid_descriptor,
                        uint16_t              hid_descriptor_len,
                        T_BT_HID_REPORT_TYPE  hid_report_type,
                        const uint8_t        *hid_report,
                        uint16_t              hid_report_len);

bool bt_hid_parser_has_more(T_BT_HID_PARSER *parser);

void bt_hid_parser_access_report_field(T_BT_HID_PARSER *parser,
                                       uint16_t        *usage_page,
                                       uint16_t        *usage,
                                       int32_t         *value);


void bt_hid_parse_descriptor_item(T_BT_HID_DESCRIPTOR_ITEM *item,
                                  const uint8_t            *hid_descriptor,
                                  uint16_t                  hid_descriptor_len);


uint32_t bt_hid_report_size_get_by_id(uint32_t              report_id,
                                      T_BT_HID_REPORT_TYPE  report_type,
                                      uint16_t              hid_descriptor_len,
                                      const uint8_t        *hid_descriptor);

T_BT_HID_REPORT_ID_STATUS bt_hid_get_report_id_status(uint32_t       report_id,
                                                      uint16_t       hid_descriptor_len,
                                                      const uint8_t *hid_descriptor);


bool bt_hid_report_id_declared(uint16_t       hid_descriptor_len,
                               const uint8_t *hid_descriptor);


bool bt_hid_report_type_is_valid(uint8_t        report_type,
                                 uint16_t       hid_descriptor_len,
                                 const uint8_t *hid_descriptor);

#ifdef __cplusplus
}
#endif

#endif /* _BT_HID_PARSER_H_ */
