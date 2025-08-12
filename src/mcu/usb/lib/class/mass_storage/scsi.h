/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file ufi.h
 * @version 1.0
 * @brief
 *
 * @note:
 */
#ifndef __SCSI_H__
#define __SCSI_H__
#include <stdint.h>

/**
 * @copyright Copyright (C) 2023 Realtek Semiconductor Corporation.
 *
 * @file scsi.h
 * @version 1.0
 * @brief
 *
 * @note:
 */

#define SCSI_FORMAT_UNIT                            0x04
#define SCSI_INQUIRY                                0x12
#define SCSI_MODE_SELECT6                           0x15
#define SCSI_MODE_SELECT10                          0x55
#define SCSI_MODE_SENSE6                            0x1A
#define SCSI_MODE_SENSE10                           0x5A
#define SCSI_ALLOW_MEDIUM_REMOVAL                   0x1E
#define SCSI_READ6                                  0x08
#define SCSI_READ10                                 0x28
#define SCSI_READ12                                 0xA8
#define SCSI_READ16                                 0x88

#define SCSI_READ_CAPACITY10                        0x25
#define SCSI_READ_CAPACITY16                        0x9E

#define SCSI_REQUEST_SENSE                          0x03
#define SCSI_START_STOP_UNIT                        0x1B
#define SCSI_TEST_UNIT_READY                        0x00
#define SCSI_WRITE6                                 0x0A
#define SCSI_WRITE10                                0x2A
#define SCSI_WRITE12                                0xAA
#define SCSI_WRITE16                                0x8A

#define SCSI_VERIFY10                               0x2F
#define SCSI_VERIFY12                               0xAF
#define SCSI_VERIFY16                               0x8F

#define SCSI_SEND_DIAGNOSTIC                        0x1D
#define SCSI_READ_FORMAT_CAPACITIES                 0x23

#define SENSE_KEY_NO_SENSE                            0
#define SENSE_KEY_RECOVERED_ERROR                     1
#define SENSE_KEY_NOT_READY                           2
#define SENSE_KEY_MEDIUM_ERROR                        3
#define SENSE_KEY_HARDWARE_ERROR                      4
#define SENSE_KEY_ILLEGAL_REQUEST                     5
#define SENSE_KEY_UNIT_ATTENTION                      6
#define SENSE_KEY_DATA_PROTECT                        7
#define SENSE_KEY_BLANK_CHECK                         8
#define SENSE_KEY_VENDOR_SPECIFIC                     9
#define SENSE_KEY_COPY_ABORTED                        10
#define SENSE_KEY_ABORTED_COMMAND                     11
#define SENSE_KEY_VOLUME_OVERFLOW                     13
#define SENSE_KEY_MISCOMPARE                          14

#define ASC_INVALID_COMMAND                             0x20
#define ASC_INVALID_FIELD_IN_CDB                        0x24
#define ASC_PARAMETER_LIST_LENGTH_ERROR                 0x1A
#define ASC_INVALID_FIELD_IN_PARAMETER_LIST             0x26
#define ASC_ADDRESS_OUT_OF_RANGE                        0x21
#define ASC_MEDIUM_NOT_PRESENT                          0x3A
#define ASC_MEDIUM_HAVE_CHANGED                         0x28

#define READ_FORMAT_CAPACITY_DATA_LEN               0x0C
#define READ_CAPACITY10_DATA_LEN                    0x08
#define MODE_SENSE10_DATA_LEN                       0x08
#define MODE_SENSE6_DATA_LEN                        0x04
#define REQUEST_SENSE_DATA_LEN                      0x12
#define STANDARD_INQUIRY_DATA_LEN                   0x24
#define BLKVFY                                      0x04

#define STATUS_BYTE_CODE_GOOD                       0x00
#define STATUS_BYTE_CODE_CHECK_CONDITION            0x01
#define STATUS_BYTE_CODE_CONDITION_MET              0x02
#define STATUS_BYTE_CODE_BUSY                       0x04
#define STATUS_BYTE_CODE_INTERMEDIATE               0x08
#define STATUS_BYTE_CODE_INTERMEDIATE_CONDITION_MET 0x0A
#define STATUS_BYTE_CODE_RESERVATION_CONFLICT       0x0C
#define STATUS_BYTE_CODE_COMMAND_TERMINATED         0x11
#define STATUS_BYTE_CODE_QUEUE_FULL                 0x14

#define GROUP_CODE(op_code)                  ((op_code) >> 5)
#define COMMAND_CODE(op_code)                ((op_code) & 0x1F)

#pragma pack(push,1)

typedef struct _t_cdb_ctrl_field
{
    uint8_t link: 1;
    uint8_t flag: 1;
    uint8_t rsv: 4;
    uint8_t vnd: 2;
} T_CDB_CTRL_FIELD;

typedef struct _t_scsi_cdb_hdr
{
    uint8_t op_code;
} T_SCSI_CDB_HDR;

typedef struct _t_scsi_cdb_6B
{
    uint8_t op_code;
    uint8_t lba_m: 5;
    uint8_t lun: 3;
    uint16_t lba_l;
    union
    {
        uint8_t xfer;
        uint8_t param_list;
        uint8_t alloc;
    } len;
    T_CDB_CTRL_FIELD ctrl;
} T_SCSI_CDB_6B;

typedef struct _t_scsi_cdb_10B
{
    uint8_t op_code;
    uint8_t rsv0: 5;
    uint8_t lun: 3;
    uint32_t lba;
    uint8_t rsv1;
    union
    {
        uint16_t xfer;
        uint16_t param_list;
        uint16_t alloc;
    } len;
    T_CDB_CTRL_FIELD ctrl;
} T_SCSI_CDB_10B;

typedef struct _t_scsi_cdb_12B
{
    uint8_t op_code;
    uint8_t rsv0: 5;
    uint8_t lun: 3;
    uint32_t lba;
    union
    {
        uint32_t xfer;
        uint32_t param_list;
        uint32_t alloc;
    } len;
    uint8_t rsv1;
    T_CDB_CTRL_FIELD ctrl;
} T_SCSI_CDB_12B;

typedef struct _t_cdb_status
{
    uint8_t rsv0: 1;
    uint8_t code: 5;
    uint8_t rsv1: 2;
} T_CDB_STATUS;

typedef struct _t_sense_data
{
    uint8_t err: 7;
    uint8_t valid: 1;
    uint8_t seg_num;
    uint8_t skey: 4;
    uint8_t rsv0: 1;
    uint8_t ili: 1;
    uint8_t eom: 1;
    uint8_t f_mark: 1;
    uint32_t info;
    uint8_t as_len;
    uint32_t cmd_specific_info;
    uint8_t asc;
    uint8_t asc_qulifier;
    uint32_t fruc: 8;
    uint32_t skey_specific0: 7;
    uint32_t sksv: 1;
    uint32_t skey_specific1: 16;
} T_SENSE_DATA;

typedef struct _mode_param_hdr_6
{
    uint8_t mode_data_len;
    uint8_t medium_type;
    uint8_t device_specific_param;
    uint8_t block_desc_len;
} T_MODE_PARAM_HDR_6;

typedef struct _mode_param_hdr_10
{
    uint16_t mode_data_len;
    uint8_t medium_type;
    uint8_t device_specific_param;
    uint16_t rsv;
    uint16_t block_desc_len;
} T_MODE_PARAM_HDR_10;

typedef struct _t_capacity_data
{
    uint32_t lba;
    uint32_t blk_len;
} T_CAPACITY_DATA;

typedef struct _t_std_inquiry_data
{
    uint8_t peripheral_dev_type: 4;
    uint8_t peripheral_qualifier: 4;

    uint8_t dev_type_modifier: 7;
    uint8_t rmb: 1;

    uint8_t ansi_ver: 3;
    uint8_t ecma_ver: 3;
    uint8_t iso_ver: 2;

    uint8_t resp_data_format: 4;
    uint8_t rsv0: 2;
    uint8_t trmiop: 1;
    uint8_t aenc: 1;

    uint8_t additional_len;
    uint16_t rsv1;

    uint8_t sftre: 1;
    uint8_t cmdque: 1;
    uint8_t rsv2: 1;
    uint8_t linked: 1;
    uint8_t sync: 1;
    uint8_t wbus16: 1;
    uint8_t wbus32: 1;
    uint8_t reladr: 1;

    uint8_t vnd_id[8];
    uint8_t prod_id[16];
    uint8_t prod_rev_lvl[4];

} T_STD_INQUIRY_DATA;

#pragma pack(pop)

#endif
