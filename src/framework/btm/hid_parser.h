/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HID_PARSER_H_
#define _HID_PARSER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum t_hid_item_type
{
    HID_ITEM_TYPE_MAIN,
    HID_ITEM_TYPE_GLOBAL,
    HID_ITEM_TYPE_LOCAL,
    HID_ITEM_TYPE_RESERVED
} T_HID_ITEM_TYPE;

typedef enum t_hid_main_item_tag
{
    HID_MAIN_ITEM_TAG_INPUT    = 0x08,
    HID_MAIN_ITEM_TAG_OUTPUT   = 0x09,
    HID_MAIN_ITEM_TAG_COLL     = 0x0a,
    HID_MAIN_ITEM_TAG_FEATURE  = 0x0b,
    HID_MAIN_ITEM_TAG_ENDCOLL  = 0x0c
} T_HID_MAIN_ITEM_TAG;

typedef enum t_hid_global_item_tag
{
    HID_GLOBAL_ITEM_TAG_USAGE_PAGE,
    HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUN,
    HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM,
    HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM,
    HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUN,
    HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT,
    HID_GLOBAL_ITEM_TAG_UNIT,
    HID_GLOBAL_ITEM_TAG_REPORT_SIZE,
    HID_GLOBAL_ITEM_TAG_REPORT_ID,
    HID_GLOBAL_ITEM_TAG_REPORT_COUNT,
    HID_GLOBAL_ITEM_TAG_PUSH,
    HID_GLOBAL_ITEM_TAG_POP
} T_HID_GLOBAL_ITEM_TAG;

typedef enum t_hid_loacl_item_tag
{
    HID_LOCAL_ITEM_TAG_USAGE,
    HID_LOCAL_ITEM_TAG_USAGE_MINIMUM,
    HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM,
    HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX,
    HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM,
    HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM,
    HID_LOCAL_ITEM_TAG_STRING_INDEX,
    HID_LOCAL_ITEM_TAG_STRING_MINIMUM,
    HID_LOCAL_ITEM_TAG_STRING_MAXIMUM,
    HID_LOCAL_ITEM_TAG_DELIMITER
} T_HID_LOCAL_ITEM_TAG;

typedef struct t_hid_descriptor_item
{
    int32_t  item_value;
    uint16_t item_size;
    uint8_t  item_type;
    uint8_t  item_tag;
    uint8_t  data_size;
} T_HID_DESCRIPTOR_ITEM;

typedef enum t_hid_parser_state
{
    HID_PARSER_STATE_SCAN_FOR_REPORT_ITEM,
    HID_PARSER_STATE_USAGES_AVAILABLE,
    HID_PARSER_STATE_COMPLETE,
} T_HID_PARSER_STATE;

typedef enum t_hid_report_type
{
    HID_REPORT_TYPE_RESERVED,
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE
} T_HID_REPORT_TYPE;

typedef enum t_hid_report_id_status
{
    HID_REPORT_ID_UNDECLARED,
    HID_REPORT_ID_VALID,
    HID_REPORT_ID_INVALID
} T_HID_REPORT_ID_STATUS;

typedef struct t_hid_parser
{
    T_HID_PARSER_STATE state;

    const uint8_t         *descriptor;
    uint16_t               descriptor_len;
    uint16_t               descriptor_pos;
    T_HID_DESCRIPTOR_ITEM  descriptor_item;

    const uint8_t     *report;
    uint16_t           report_len;
    T_HID_REPORT_TYPE  report_type;
    uint16_t           report_pos_in_bit;

    uint16_t        usage_pos;
    uint16_t        usage_page;

    uint32_t        usage_minimum;
    uint32_t        usage_maximum;
    uint16_t        available_usages;
    uint8_t         required_usages;
    uint8_t         active_record;
    uint8_t         have_usage_min;
    uint8_t         have_usage_max;

    int32_t         global_logical_minimum;
    int32_t         global_logical_maximum;
    uint16_t        global_usage_page;
    uint8_t         global_report_size;
    uint8_t         global_report_count;
    uint8_t         global_report_id;
} T_HID_PARSER;

void hid_parser_init(T_HID_PARSER      *parser,
                     const uint8_t     *hid_descriptor,
                     uint16_t           hid_descriptor_len,
                     T_HID_REPORT_TYPE  hid_report_type,
                     const uint8_t     *hid_report,
                     uint16_t           hid_report_len);

bool hid_parser_has_more(T_HID_PARSER *parser);

void hid_parser_access_report_field(T_HID_PARSER *parser,
                                    uint16_t     *usage_page,
                                    uint16_t     *usage,
                                    int32_t      *value);


void hid_parse_descriptor_item(T_HID_DESCRIPTOR_ITEM *item,
                               const uint8_t         *hid_descriptor,
                               uint16_t               hid_descriptor_len);


uint32_t hid_report_size_get_by_id(uint32_t           report_id,
                                   T_HID_REPORT_TYPE  report_type,
                                   uint16_t           hid_descriptor_len,
                                   const uint8_t     *hid_descriptor);

T_HID_REPORT_ID_STATUS hid_get_report_id_status(uint32_t       report_id,
                                                uint16_t       hid_descriptor_len,
                                                const uint8_t *hid_descriptor);


bool hid_report_id_declared(uint16_t       hid_descriptor_len,
                            const uint8_t *hid_descriptor);


bool hid_report_type_is_valid(uint8_t        report_type,
                              uint16_t       hid_descriptor_len,
                              const uint8_t *hid_descriptor);

#ifdef __cplusplus
}
#endif

#endif /* _HID_PARSER_H_ */
