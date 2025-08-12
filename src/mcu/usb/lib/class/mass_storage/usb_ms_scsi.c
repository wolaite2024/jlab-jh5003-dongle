#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "errno.h"
#include "rtl876x.h"
#include "usb_utils.h"
#include "usb_ms_driver.h"
#include "scsi.h"

#define BLK_CNT_PER_READ        (0x8)
#define BLK_CNT_PER_WRITE       (0x02)

static T_USB_UTILS_LIST g_sense_list;
static uint8_t g_prev_cmd = 0xff;

static uint32_t g_lba_max = 0;
static uint32_t g_blk_size = 0;
static uint32_t g_next_lba = 0;
static uint32_t g_remaining_blk = 0;
static uint32_t fail_line = 0;

typedef struct _t_sense_item
{
    struct _t_sense_item *p_next;
    T_SENSE_DATA data;
} T_SENSE_ITEM;

static int usb_ms_scsi_sense_code(uint8_t skey, uint8_t asc)
{
    T_SENSE_ITEM *sense_item = malloc(sizeof(T_SENSE_ITEM));
    if (sense_item == NULL)
    {
        return -ENOMEM;
    }

    memset(sense_item, 0, sizeof(T_SENSE_ITEM));
    sense_item->data.skey = skey;
    sense_item->data.asc = asc;
    USB_UTILS_LIST_INSERT_TAIL(&g_sense_list, sense_item);
    return ESUCCESS;

}

static int usb_ms_scsi_cmd_proc_capacity_read10(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb)
{
    T_CAPACITY_DATA capacity = {.lba = 0, .blk_len = 0};
    int ret = 0;

    if ((!disk->is_ready) || !disk->is_ready())
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
        return -ENOTSUPP;
    }

    if (disk->capacity_get)
    {
        ret = disk->capacity_get(&g_lba_max, &g_blk_size);
        if (ret < 0)
        {
            return ret;
        }
    }
    capacity.lba = rtk_cpu_to_be32(g_lba_max);
    capacity.blk_len = rtk_cpu_to_be32(g_blk_size);

    ret = usb_ms_driver_data_pipe_send(&capacity, sizeof(T_CAPACITY_DATA), NULL);

    return ret;
}

static int usb_ms_scsi_cmd_proc_inquiry(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb, uint32_t h_len)
{
    T_STD_INQUIRY_DATA inquiry_data;
    uint32_t len = sizeof(T_STD_INQUIRY_DATA);
    int ret = 0;

    if (h_len < len)
    {
        len = h_len;
    }
//    if(cdb_inquiry->evpd)
//    {
//        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CDB);
//        return -EOPNOTSUPP;
//    }

    memset(&inquiry_data, 0, sizeof(T_STD_INQUIRY_DATA));
    inquiry_data.peripheral_dev_type = 0;
    inquiry_data.rmb = 1;
    inquiry_data.ansi_ver = 2;
    inquiry_data.additional_len = sizeof(T_STD_INQUIRY_DATA) - 4;
    uint8_t vnd_id[] = {'R', 'e', 'a', 'l', 't', 'e', 'k', ' '};
    memcpy(inquiry_data.vnd_id, vnd_id, sizeof(inquiry_data.vnd_id));
    uint8_t prod_id[] = {'M', 'a', 's', 's', ' ', 's', 't', 'o', 'r', 'a', 'g', 'e', ' ', ' ', ' ', ' '};
    memcpy(inquiry_data.prod_id, prod_id, sizeof(inquiry_data.prod_id));

    ret = usb_ms_driver_data_pipe_send(&inquiry_data, len, NULL);

    return ret;
}

static int usb_ms_scsi_proc_mode_sense6(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb, uint32_t h_len)
{
    int ret = 0;
    uint32_t len = sizeof(T_MODE_PARAM_HDR_6);

    if (h_len < len)
    {
        len = h_len;
    }

    T_MODE_PARAM_HDR_6 sense6_data =
    {
        .mode_data_len = 3,
        .medium_type = 0,
        .device_specific_param = 0,
        .block_desc_len = 0,
    };

    ret = usb_ms_driver_data_pipe_send(&sense6_data, len, NULL);

    return ret;
}


static int usb_ms_scsi_proc_request_sense(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb, uint32_t h_len)
{
    T_SENSE_ITEM *sense_item = NULL;
    int ret = ESUCCESS;
    uint32_t len = sizeof(T_SENSE_DATA);

    if (h_len < len)
    {
        len = h_len;
    }

    T_SENSE_DATA sense_data;
    memset(&sense_data, 0, sizeof(T_SENSE_DATA));
    sense_data.valid = 1;
    sense_data.err = 0x70;
    sense_data.as_len = sizeof(T_SENSE_DATA) - 7;

    USB_UTILS_LIST_REMOVE_HEAD(&g_sense_list, sense_item);
    if (sense_item)
    {
        sense_data.asc = sense_item->data.asc;
        sense_data.skey = sense_item->data.skey;

        ret = usb_ms_driver_data_pipe_send(&sense_data, len, NULL);
        free(sense_item);
    }
    else
    {
        sense_data.asc = SENSE_KEY_NO_SENSE;

        ret = usb_ms_driver_data_pipe_send(&sense_data, len, NULL);
    }

    return ret;
}

static bool usb_ms_scsi_address_check(T_DISK_DRIVER *disk, uint8_t lun, uint32_t lba,
                                      uint32_t blk_num)
{
    return ((lba + blk_num) <= (g_lba_max + 1));
}

static int usb_ms_scsi_tx_xfer_done(T_DISK_DRIVER *disk, uint32_t actual, int status)
{
    uint16_t blk_cnt = actual / g_blk_size;
    uint16_t blk_per_read = 0;
    int ret = ESUCCESS;
    USB_PRINT_INFO3("usb_ms_scsi_tx_xfer_done:%d-%d-%d", g_prev_cmd, g_remaining_blk, g_next_lba);
    switch (g_prev_cmd)
    {
    case SCSI_READ10:
        {
            g_remaining_blk -= blk_cnt;
            if (g_remaining_blk > BLK_CNT_PER_READ)
            {
                blk_per_read = BLK_CNT_PER_READ;
            }
            else
            {
                blk_per_read = g_remaining_blk;
                g_prev_cmd = 0xFF;
            }

            if (blk_per_read)
            {

                if (!disk->buffer_alloc)
                {
                    usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
                    fail_line = __LINE__;
                    ret = -ENOTSUPP;
                    goto end;
                }
                uint8_t *buf = disk->buffer_alloc(blk_per_read);
                if (buf == NULL)
                {
                    fail_line = __LINE__;
                    ret = -ENOMEM;
                    goto end;
                }
                ret = disk->read(g_next_lba, blk_per_read, buf);
                g_next_lba += blk_per_read;
                ret += usb_ms_driver_data_pipe_send(buf, g_blk_size * blk_per_read, usb_ms_scsi_tx_xfer_done);
                if (disk->buffer_free)
                {
                    disk->buffer_free(buf);
                }
            }

        }
        break;

    default:

        break;
    }

end:
    return ret;
}

static int usb_ms_scsi_proc_read10(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb, uint32_t h_len,
                                   uint8_t dir)
{
    T_SCSI_CDB_10B *cdb_read10 = (T_SCSI_CDB_10B *)cdb;
    uint32_t lba = rtk_be32_to_cpu(cdb_read10->lba);
    uint16_t blk_cnt = rtk_be16_to_cpu(cdb_read10->len.xfer);
    uint16_t len = blk_cnt * g_blk_size;
    uint16_t blk_per_read = 0;
    int ret = 0;

    USB_PRINT_INFO4("usb_ms_scsi_proc_read10, len %d, blk_cnt %d, g_blk_size %d, h_len %d", len,
                    blk_cnt, g_blk_size, h_len);

    if ((dir & USB_DIR_MASK) == USB_DIR_OUT)
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
        return -EINVAL;
    }

    if (h_len == 0 && h_len != len)
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CDB);
        ret = -EINVAL;
        goto end;
    }
    else if (h_len > len)
    {
        blk_cnt = h_len / g_blk_size;
    }
    else if (h_len < len)
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CDB);
        fail_line = __LINE__;
        ret = -EINVAL;
        goto end;
    }

    if (!usb_ms_scsi_address_check(disk, 0, lba, blk_cnt))
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
        fail_line = __LINE__;
        ret = -EINVAL;
        goto end;
    }

    if ((!disk->is_ready) || !disk->is_ready())
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
        fail_line = __LINE__;
        ret = -ENOTSUPP;
        goto end;
    }
    g_remaining_blk = blk_cnt;
    g_next_lba = lba;
    if (disk->read)
    {
        if (g_remaining_blk > BLK_CNT_PER_READ)
        {
            g_prev_cmd = SCSI_READ10;
            blk_per_read = BLK_CNT_PER_READ;
        }
        else
        {
            blk_per_read = g_remaining_blk;
        }

        if (!disk->buffer_alloc)
        {
            usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
            fail_line = __LINE__;
            ret = -ENOTSUPP;
            goto end;
        }
        uint8_t *buf = disk->buffer_alloc(blk_per_read);
        if (buf == NULL)
        {
            fail_line = __LINE__;
            ret = -ENOMEM;
            goto end;
        }
        memset(buf, 0, g_blk_size * blk_per_read);
        ret = disk->read(g_next_lba, blk_per_read, buf);
        g_next_lba += blk_per_read;
        USB_PRINT_INFO4("usb_ms_scsi_proc_read10:%d-%d-%d-%d", buf, lba, blk_per_read, len);

        ret += usb_ms_driver_data_pipe_send(buf, g_blk_size * blk_per_read, usb_ms_scsi_tx_xfer_done);

        if (disk->buffer_free)
        {
            disk->buffer_free(buf);
        }

    }
    else
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
        fail_line = __LINE__;
        ret = -ENOTSUPP;
        goto end;
    }

end:
    USB_PRINT_INFO2("usb_ms_scsi_proc_read10, fail_line :%d, ret: %d", fail_line, ret);
    return ret;
}

static int usb_ms_scsi_proc_write10(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb, uint32_t h_len,
                                    uint8_t dir)
{
    T_SCSI_CDB_10B *cdb_write10 = (T_SCSI_CDB_10B *)cdb;
    uint32_t lba = rtk_be32_to_cpu(cdb_write10->lba);
    uint16_t blk_cnt = rtk_be16_to_cpu(cdb_write10->len.xfer);
    uint16_t len = blk_cnt * g_blk_size;
    int ret = 0;
    USB_PRINT_INFO5("usb_ms_scsi_proc_write10:%d-%d-%d-%d-%d", lba, blk_cnt, g_lba_max, h_len, len);

    if ((dir & USB_DIR_MASK) == USB_DIR_IN)
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
        return -EINVAL;
    }

    if ((h_len == 0 && h_len != len))
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CDB);
        return -EINVAL;
    }
    else if (h_len > len)
    {
        blk_cnt = h_len / g_blk_size;
    }
    else if (h_len < len)
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CDB);
        fail_line = __LINE__;
        ret = -EINVAL;
        goto end;
    }

    if (!usb_ms_scsi_address_check(disk, 0, lba, blk_cnt))
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_ADDRESS_OUT_OF_RANGE);
        return -EINVAL;
    }

    if ((!disk->is_ready) || !disk->is_ready() || (!disk->write))
    {
        usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
        return -ENOTSUPP;
    }
    g_prev_cmd = SCSI_WRITE10;
    g_next_lba = lba;
    g_remaining_blk = blk_cnt;
end:
    return ret;
}

static int usb_ms_scsi_proc_data_rx(T_DISK_DRIVER *disk, uint8_t *data, uint32_t len)
{
    uint16_t blk_cnt = len / g_blk_size;
    int ret = ESUCCESS;
    USB_PRINT_INFO3("usb_ms_scsi_proc_data_rx:%d-%d-%d", g_next_lba, len, g_remaining_blk);
    switch (g_prev_cmd)
    {
    case SCSI_WRITE10:
        {
            disk->write(g_next_lba, blk_cnt, data);
            g_next_lba += blk_cnt;
            g_remaining_blk -= blk_cnt;
            if (g_remaining_blk == 0)
            {
                g_prev_cmd = 0xFF;
            }
        }
        break;

    default:
        {
            ret = -ENOTSUPP;
        }
        break;
    }
    return ret;
}

static int usb_ms_scsi_cmd_test_unit_ready(T_DISK_DRIVER *disk, T_SCSI_CDB_HDR *cdb)
{
    return ESUCCESS;
}

static int usb_ms_scsi_cmd_proc(T_DISK_DRIVER *disk, T_CBW *cbw)
{
    T_SCSI_CDB_HDR *cdb = (T_SCSI_CDB_HDR *)cbw->CBWCB;
    uint32_t h_len = cbw->dCBWDataTransferLength;
    uint8_t dir = cbw->bmCBWFlags;
    uint8_t cmd = cdb->op_code;
    int ret = ESUCCESS;

    switch (cmd)
    {
    case SCSI_TEST_UNIT_READY:
        {
            ret = usb_ms_scsi_cmd_test_unit_ready(disk, cdb);
        }
        break;

    case SCSI_INQUIRY:
        {
            ret = usb_ms_scsi_cmd_proc_inquiry(disk, cdb, h_len);
        }
        break;

    case SCSI_READ_CAPACITY10:
        {
            ret = usb_ms_scsi_cmd_proc_capacity_read10(disk, cdb);
        }
        break;

    case SCSI_READ10:
        {
            ret = usb_ms_scsi_proc_read10(disk, cdb, h_len, dir);
        }
        break;

    case SCSI_WRITE10:
        {
            ret = usb_ms_scsi_proc_write10(disk, cdb, h_len, dir);
        }
        break;

    case SCSI_REQUEST_SENSE:
        {
            ret = usb_ms_scsi_proc_request_sense(disk, cdb, h_len);
        }
        break;

    case SCSI_MODE_SENSE6:
        {
            ret = usb_ms_scsi_proc_mode_sense6(disk, cdb, h_len);
        }
        break;

    default:
        {
            usb_ms_scsi_sense_code(SENSE_KEY_ILLEGAL_REQUEST, ASC_INVALID_COMMAND);
            ret = -ENOTSUPP;
        }
        break;
    }
    USB_PRINT_INFO2("usb_ms_scsi_cmd_proc, cmd: 0x%x, ret: %d", cmd, ret);
    return ret;
}


int usb_ms_scsi_init(void)
{
    T_RX_CB cbs = {.cmd = usb_ms_scsi_cmd_proc, .data = usb_ms_scsi_proc_data_rx};
    usb_ms_driver_data_pipe_ind_cbs_register(&cbs);
    return ESUCCESS;
}
