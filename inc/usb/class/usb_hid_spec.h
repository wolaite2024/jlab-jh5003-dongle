#ifndef __USB_HID_SPEC_H__
#define __USB_HID_SPEC_H__

/*
 * HID brequest field value
 */
#define HID_REQ_GET_REPORT              0x01
#define HID_REQ_GET_IDLE                0x02
#define HID_REQ_GET_PROTOCOL            0x03
#define HID_REQ_SET_REPORT              0x09
#define HID_REQ_SET_IDLE                0x0A
#define HID_REQ_SET_PROTOCOL            0x0B

#define HID_DT_HID                      (USB_TYPE_CLASS | 0x01)
#define HID_DT_REPORT                   (USB_TYPE_CLASS | 0x02)

/*
 * HID report descriptor item type (prefix bit 2,3)
 */

#define HID_ITEM_TYPE_MAIN              0x00
#define HID_ITEM_TYPE_GLOBAL            0x01
#define HID_ITEM_TYPE_LOCAL             0x02
#define HID_ITEM_TYPE_RESERVED          0x03

/*
 * HID report descriptor main item tags (prefix bit 4,7)
 */

#define HID_MAIN_ITEM_TAG_INPUT                 0x08
#define HID_MAIN_ITEM_TAG_OUTPUT                0x09
#define HID_MAIN_ITEM_TAG_FEATURE               0x0b
#define HID_MAIN_ITEM_TAG_COLLECTION            0x0a
#define HID_MAIN_ITEM_TAG_END_COLLECTION        0x0c

/*
 * HID report descriptor collection item types
 */

#define HID_COLLECTION_PHYSICAL         0x00
#define HID_COLLECTION_APPLICATION      0x01
#define HID_COLLECTION_LOGICAL          0x02
#define HID_COLLECTION_REPORT           0x03
#define HID_COLLECTION_NAMED_ARRAY      0x04
#define HID_COLLECTION_USAGE_SWITCH     0x03
#define HID_COLLECTION_USAGE_MODIFIER   0x04

/*
 * HID report descriptor global item tags
 */

#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE          0x00
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM     0x01
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM     0x02
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM    0x03
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM    0x04
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT       0x05
#define HID_GLOBAL_ITEM_TAG_UNIT                0x06
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE         0x07
#define HID_GLOBAL_ITEM_TAG_REPORT_ID           0x08
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT        0x09
#define HID_GLOBAL_ITEM_TAG_PUSH                0x0a
#define HID_GLOBAL_ITEM_TAG_POP                 0x0b

/*
 * HID report descriptor local item tags
 */

#define HID_LOCAL_ITEM_TAG_USAGE                0x00
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM        0x01
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM        0x02
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX     0x03
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM   0x04
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM   0x05
#define HID_LOCAL_ITEM_TAG_STRING_INDEX         0x07
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM       0x08
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM       0x09
#define HID_LOCAL_ITEM_TAG_DELIMITER            0x0a



#pragma pack(push,1)
typedef struct _hid_class_descriptor
{
    uint8_t  bDescriptorType;
    uint16_t wDescriptorLength;
} T_HID_CLASS_DESC;

typedef struct _hid_cs_if_desc
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;

    T_HID_CLASS_DESC desc[1];
} T_HID_CS_IF_DESC;
#pragma pack(pop)


#endif
