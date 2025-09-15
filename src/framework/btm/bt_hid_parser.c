/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#if (CONFIG_REALTEK_BTM_HID_DEVICE_SUPPORT == 1 || CONFIG_REALTEK_BTM_HID_HOST_SUPPORT == 1)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "trace.h"
#include "bt_hid_parser.h"

void bt_hid_parse_descriptor_item(T_BT_HID_DESCRIPTOR_ITEM *item,
                                  const uint8_t            *hid_descriptor,
                                  uint16_t                  hid_descriptor_len)
{
    uint16_t pos = 0;
    uint8_t item_header = hid_descriptor[pos++];
    uint8_t i = 0;
    int32_t read_value = 0;
    uint8_t next_byte = 0;

    if (hid_descriptor_len < 1)
    {
        return;
    }

    item->item_type = (item_header & 0x0c) >> 2;
    item->item_tag  = (item_header & 0xf0) >> 4;
    item->item_value = 0;

    switch (item_header & 0x03)
    {
    case 0:
        item->data_size = 0;
        break;

    case 1:
        item->data_size = 1;
        break;

    case 2:
        item->data_size = 2;
        if ((item->item_tag == 0x0f) && (item->item_type == BT_HID_ITEM_TYPE_RESERVED))
        {
            if (hid_descriptor_len < 3)
            {
                return;
            }
            item->data_size = hid_descriptor[pos++];
            item->item_tag  = hid_descriptor[pos++];
        }
        break;

    case 3:
        item->data_size = 4;
        break;
    }

    item->item_size =  pos + item->data_size;
    if (hid_descriptor_len >= item->item_size && item->data_size <= 4)
    {
        for (i = 0; i < item->data_size; i++)
        {
            next_byte = hid_descriptor[pos++];
            read_value = (next_byte << (8 * i)) | read_value;
        }
        // HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUN ~ HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUN are signed int
        if ((item->item_type == BT_HID_ITEM_TYPE_GLOBAL) &&
            (item->item_tag >= BT_HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUN) &&
            (item->item_tag <= BT_HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUN))
        {
            if (item->data_size > 0 && (next_byte >> 7))
            {
                read_value -= 1 << (item->data_size * 8);
            }
        }
        item->item_value = read_value;
    }
}

static void bt_hid_find_next_usage(T_BT_HID_PARSER *parser)
{
    while ((parser->available_usages == 0) && (parser->usage_pos < parser->descriptor_pos))
    {
        T_BT_HID_DESCRIPTOR_ITEM usage_item;
        bt_hid_parse_descriptor_item(&usage_item, &parser->descriptor[parser->usage_pos],
                                     parser->descriptor_len - parser->usage_pos);
        if ((usage_item.item_type == BT_HID_ITEM_TYPE_GLOBAL) &&
            (usage_item.item_tag == BT_HID_GLOBAL_ITEM_TAG_USAGE_PAGE))
        {
            parser->usage_page = usage_item.item_value;
        }
        if (usage_item.item_type == BT_HID_ITEM_TYPE_LOCAL)
        {
            uint32_t usage_value;
            if (usage_item.data_size > 2)
            {
                usage_value = usage_item.item_value;
            }
            else
            {
                usage_value = (parser->usage_page << 16) | usage_item.item_value;
            }

            switch (usage_item.item_tag)
            {
            case BT_HID_LOCAL_ITEM_TAG_USAGE:
                parser->available_usages = 1;
                parser->usage_minimum = usage_value;
                break;

            case BT_HID_LOCAL_ITEM_TAG_USAGE_MINIMUM:
                parser->usage_minimum = usage_value;
                parser->have_usage_min = 1;
                break;

            case BT_HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM:
                parser->usage_maximum = usage_value;
                parser->have_usage_max = 1;
                break;

            default:
                break;
            }
            if (parser->have_usage_min && parser->have_usage_max)
            {
                parser->available_usages = parser->usage_maximum - parser->usage_minimum + 1;
                parser->have_usage_min = 0;
                parser->have_usage_max = 0;
            }
        }
        parser->usage_pos += usage_item.item_size;
    }
}

static void bt_hid_process_item(T_BT_HID_PARSER          *parser,
                                T_BT_HID_DESCRIPTOR_ITEM *item)
{
    bool report_type_valid = false;

    switch ((T_BT_HID_ITEM_TYPE)item->item_type)
    {
    case BT_HID_ITEM_TYPE_MAIN:
        switch (item->item_tag)
        {
        case BT_HID_MAIN_ITEM_TAG_INPUT:
            if (parser->report_type == BT_HID_REPORT_TYPE_INPUT)
            {
                report_type_valid = true;
            }
            break;

        case BT_HID_MAIN_ITEM_TAG_OUTPUT:
            if (parser->report_type == BT_HID_REPORT_TYPE_OUTPUT)
            {
                report_type_valid = true;
            }
            break;

        case BT_HID_MAIN_ITEM_TAG_FEATURE:
            if (parser->report_type == BT_HID_REPORT_TYPE_FEATURE)
            {
                report_type_valid = true;
            }
            break;

        default:
            break;
        }
        break;

    case BT_HID_ITEM_TYPE_GLOBAL:
        switch (item->item_tag)
        {
        case BT_HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
            parser->global_usage_page = item->item_value;
            break;

        case BT_HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUN:
            parser->global_logical_minimum = item->item_value;
            break;

        case BT_HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
            parser->global_logical_maximum = item->item_value;
            break;

        case BT_HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
            parser->global_report_size = item->item_value;
            break;

        case BT_HID_GLOBAL_ITEM_TAG_REPORT_ID:
            if (parser->active_record && (parser->global_report_id != item->item_value))
            {
                parser->active_record = 0;
            }
            parser->global_report_id = item->item_value;
            break;

        case BT_HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
            parser->global_report_count = item->item_value;
            break;

        default:
            break;
        }
        break;

    case BT_HID_ITEM_TYPE_LOCAL:
    case BT_HID_ITEM_TYPE_RESERVED:
    default:
        break;
    }

    if (report_type_valid)
    {
        if (parser->global_report_id && !parser->active_record)
        {
            if (parser->report[0] != parser->global_report_id)
            {
                return;
            }
            parser->report_pos_in_bit += 8;
        }
        parser->active_record = 1;
        if (item->item_value & 1)
        {
            parser->report_pos_in_bit += parser->global_report_size * parser->global_report_count;
            return;
        }

        if (parser->global_report_count != 0)
        {
            parser->required_usages = parser->global_report_count;
        }
    }

}

static void bt_hid_post_process_item(T_BT_HID_PARSER          *parser,
                                     T_BT_HID_DESCRIPTOR_ITEM *item)
{
    if ((T_BT_HID_ITEM_TYPE)item->item_type == BT_HID_ITEM_TYPE_MAIN)
    {
        parser->usage_pos  = parser->descriptor_pos;
        parser->usage_page = parser->global_usage_page;
    }
    parser->descriptor_pos += item->item_size;
}

static void bt_hid_parser_find_next_usage(T_BT_HID_PARSER *parser)
{
    while (parser->state == BT_HID_PARSER_STATE_SCAN_FOR_REPORT_ITEM)
    {
        if (parser->descriptor_pos >= parser->descriptor_len)
        {
            // end of descriptor
            parser->state = BT_HID_PARSER_STATE_COMPLETE;
            break;
        }
        bt_hid_parse_descriptor_item(&parser->descriptor_item,
                                     &parser->descriptor[parser->descriptor_pos], parser->descriptor_len - parser->descriptor_pos);
        bt_hid_process_item(parser, &parser->descriptor_item);
        if (parser->required_usages)
        {
            bt_hid_find_next_usage(parser);
            if (parser->available_usages)
            {
                parser->state = BT_HID_PARSER_STATE_USAGES_AVAILABLE;
            }
            else
            {
                PROFILE_PRINT_ERROR0("no usages found");
                parser->state = BT_HID_PARSER_STATE_COMPLETE;
            }
        }
        else
        {
            bt_hid_post_process_item(parser, &parser->descriptor_item);
        }
    }
}

void bt_hid_parser_init(T_BT_HID_PARSER      *parser,
                        const uint8_t        *hid_descriptor,
                        uint16_t              hid_descriptor_len,
                        T_BT_HID_REPORT_TYPE  hid_report_type,
                        const uint8_t        *hid_report,
                        uint16_t              hid_report_len)
{

    memset(parser, 0, sizeof(T_BT_HID_PARSER));

    parser->descriptor     = hid_descriptor;
    parser->descriptor_len = hid_descriptor_len;
    parser->report_type    = hid_report_type;
    parser->report         = hid_report;
    parser->report_len     = hid_report_len;
    parser->state          = BT_HID_PARSER_STATE_SCAN_FOR_REPORT_ITEM;

    bt_hid_parser_find_next_usage(parser);
}

bool bt_hid_parser_has_more(T_BT_HID_PARSER *parser)
{
    if (parser->state == BT_HID_PARSER_STATE_USAGES_AVAILABLE)
    {
        return true;
    }
    return false;
}

void bt_hid_parser_access_report_field(T_BT_HID_PARSER *parser,
                                       uint16_t        *usage_page,
                                       uint16_t        *usage,
                                       int32_t         *value)
{
    uint8_t is_variable = 0;
    uint32_t pos_start = 0;
    uint32_t pos_end = 0;
    uint8_t bytes_to_read = 0;
    uint32_t read_data = 0;
    uint32_t tmp_value = 0;
    uint8_t extra_bit_num = 0;
    uint8_t i = 0;

    pos_start = parser->report_pos_in_bit >> 3;
    pos_end = (parser->report_pos_in_bit + parser->global_report_size - 1) >> 3;
    *usage_page = parser->usage_minimum >> 16;
    // read field (up to 32 bit unsigned, up to 31 bit signed - 32 bit signed behaviour is undefined) - check report len
    // whether it is a variable
    if (parser->descriptor_item.item_value & 0x10)
    {
        is_variable =  1;
    }

    if (pos_start > parser->report_len)
    {
        pos_start = parser->report_len;
    }

    if (pos_end > parser->report_len)
    {
        pos_end = parser->report_len;
    }

    bytes_to_read = pos_end - pos_start + 1;

    for (i = 0; i < bytes_to_read; i++)
    {
        read_data |= parser->report[pos_start + i] << (i * 8);
    }
    extra_bit_num = parser->report_pos_in_bit % 8;
    tmp_value = (read_data >> extra_bit_num) & ((1 << parser->global_report_size) - 1);
    if (is_variable)
    {
        *usage      = parser->usage_minimum & 0xffff;
        //// whether it is a signed int
        if (parser->global_logical_minimum < 0 &&
            (tmp_value & (1 << (parser->global_report_size - 1))))
        {
            *value = tmp_value - (1 << parser->global_report_size);
        }
        else
        {
            *value = tmp_value;
        }
    }
    else
    {
        *usage  = tmp_value;
        *value  = 1;
    }
    parser->required_usages--;
    parser->report_pos_in_bit += parser->global_report_size;

    if (is_variable)
    {
        parser->usage_minimum++;
        parser->available_usages--;
    }
    else
    {
        if (parser->required_usages == 0)
        {
            parser->available_usages = 0;
        }
    }
    if (parser->available_usages)
    {
        return;
    }
    if (parser->required_usages == 0)
    {
        bt_hid_post_process_item(parser, &parser->descriptor_item);
        parser->state = BT_HID_PARSER_STATE_SCAN_FOR_REPORT_ITEM;
        bt_hid_parser_find_next_usage(parser);
    }
    else
    {
        bt_hid_find_next_usage(parser);
        if (parser->available_usages == 0)
        {
            parser->state = BT_HID_PARSER_STATE_COMPLETE;
        }
    }
}

uint32_t bt_hid_report_size_get_by_id(uint32_t              report_id,
                                      T_BT_HID_REPORT_TYPE  report_type,
                                      uint16_t              hid_descriptor_len,
                                      const uint8_t        *hid_descriptor)
{
    uint32_t total_report_size = 0;
    uint32_t current_report_size = 0;
    uint32_t current_report_count = 0;
    uint32_t report_size = 0;
    uint32_t report_count = 0;
    uint32_t current_report_id = 0;

    while (hid_descriptor_len)
    {
        uint32_t current_report_type = 0;
        T_BT_HID_DESCRIPTOR_ITEM item;
        bt_hid_parse_descriptor_item(&item, hid_descriptor, hid_descriptor_len);

        switch (item.item_type)
        {
        case BT_HID_ITEM_TYPE_GLOBAL:
            switch ((T_BT_HID_GLOBAL_ITEM_TAG)item.item_tag)
            {
            case BT_HID_GLOBAL_ITEM_TAG_REPORT_ID:
                current_report_id = item.item_value;
                break;

            case BT_HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
                current_report_count = item.item_value;
                break;

            case BT_HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
                current_report_size = item.item_value;
                break;

            default:
                break;
            }
            break;

        case BT_HID_ITEM_TYPE_MAIN:
            if (current_report_id == report_id)
            {
                report_size = current_report_size;
                report_count = current_report_count;

                switch ((T_BT_HID_MAIN_ITEM_TAG)item.item_tag)
                {
                case BT_HID_MAIN_ITEM_TAG_INPUT:
                    current_report_type = BT_HID_REPORT_TYPE_INPUT;
                    break;

                case BT_HID_MAIN_ITEM_TAG_OUTPUT:
                    current_report_type = BT_HID_REPORT_TYPE_OUTPUT;
                    break;

                case BT_HID_MAIN_ITEM_TAG_FEATURE:
                    current_report_type = BT_HID_REPORT_TYPE_FEATURE;
                    break;

                default:
                    break;
                }
                if (report_type == current_report_type)
                {
                    total_report_size += report_count * report_size;
                    report_size = 0;
                    report_count = 0;
                }
            }
            break;

        default:
            break;
        }
        hid_descriptor_len -= item.item_size;
        hid_descriptor += item.item_size;
    }
    return (total_report_size + 7) / 8;
}

T_BT_HID_REPORT_ID_STATUS bt_hid_get_report_id_status(uint32_t       report_id,
                                                      uint16_t       hid_descriptor_len,
                                                      const uint8_t *hid_descriptor)
{
    uint32_t current_report_id = 0;

    while (hid_descriptor_len)
    {
        T_BT_HID_DESCRIPTOR_ITEM item;

        bt_hid_parse_descriptor_item(&item, hid_descriptor, hid_descriptor_len);

        if (item.item_type == BT_HID_ITEM_TYPE_GLOBAL &&
            (T_BT_HID_GLOBAL_ITEM_TAG)item.item_tag == BT_HID_GLOBAL_ITEM_TAG_REPORT_ID)
        {
            current_report_id = item.item_value;
            if (report_id == current_report_id)
            {
                return BT_HID_REPORT_ID_VALID;
            }
        }

        hid_descriptor_len -= item.item_size;
        hid_descriptor += item.item_size;
    }

    if (current_report_id)
    {
        return BT_HID_REPORT_ID_INVALID;
    }

    return BT_HID_REPORT_ID_UNDECLARED;
}

bool bt_hid_report_id_declared(uint16_t       hid_descriptor_len,
                               const uint8_t *hid_descriptor)
{
    while (hid_descriptor_len)
    {
        T_BT_HID_DESCRIPTOR_ITEM item;
        bt_hid_parse_descriptor_item(&item, hid_descriptor, hid_descriptor_len);

        if (item.item_type == BT_HID_ITEM_TYPE_GLOBAL &&
            (T_BT_HID_GLOBAL_ITEM_TAG)item.item_tag == BT_HID_GLOBAL_ITEM_TAG_REPORT_ID)
        {
            return true;
        }

        hid_descriptor_len -= item.item_size;
        hid_descriptor += item.item_size;
    }
    return false;
}

bool bt_hid_report_type_is_valid(uint8_t        report_type,
                                 uint16_t       hid_descriptor_len,
                                 const uint8_t *hid_descriptor)
{
    while (hid_descriptor_len)
    {
        T_BT_HID_DESCRIPTOR_ITEM item;
        bt_hid_parse_descriptor_item(&item, hid_descriptor, hid_descriptor_len);

        switch (item.item_type)
        {
        case BT_HID_ITEM_TYPE_MAIN:
            switch ((T_BT_HID_MAIN_ITEM_TAG)item.item_tag)
            {
            case BT_HID_MAIN_ITEM_TAG_INPUT:
                if (report_type == BT_HID_REPORT_TYPE_INPUT)
                {
                    return true;
                }
                break;

            case BT_HID_MAIN_ITEM_TAG_OUTPUT:
                if (report_type == BT_HID_REPORT_TYPE_OUTPUT)
                {
                    return true;
                }
                break;

            case BT_HID_MAIN_ITEM_TAG_FEATURE:
                if (report_type == BT_HID_REPORT_TYPE_FEATURE)
                {
                    return true;
                }
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
        hid_descriptor_len -= item.item_size;
        hid_descriptor += item.item_size;
    }
    return false;
}
#endif
