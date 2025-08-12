/**
*****************************************************************************************
*     Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file
   * @brief
   * @author    Grace
   * @date      2021-08-03
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2021 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/

#include <string.h>
#include <bt_types.h>
#include "dfu_api.h"
#include "dfu_common.h"
#include "fmc_api.h"
#include "trace.h"
#include "sha256.h"
#include "rtl876x.h"
#include "os_sync.h"


/** @defgroup  APP_OTA_SERVICE APP OTA handle
    * @brief APP OTA Service to implement OTA feature
    * @{
    */
#define SHA256_LENGTH                   32
#define SHA256_BUFFER_SIZE              128
#define READ_BACK_BUFFER_SIZE           64

#define BTAON_FAST_REBOOT_SW_INFO0      0x2
#define NOT_READY_OFFSET                308
#define CFU_PACKET_SIZE                 52
/*max img num = 32, BIT0: OTA, BIT1: SecureBoot, ..., */
uint32_t valid_bitmap = 0;
PLATFORM_STATIC_ASSERT((IMAGE_MAX - IMG_OTA + 1) < (sizeof(valid_bitmap) << 3), valid_bitmap);
/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @defgroup APP_OTA_Exported_Variables APP OTA Exported Variables
    * @brief
    * @{
*/


/** End of APP_OTA_Exported_Variables
    * @}
    */

/*============================================================================*
 *                              Types
 *============================================================================*/
/* 0x02 (BTAON_FAST_REBOOT_SW_INFO0) */
typedef union _BTAON_FAST_REBOOT_SW_INFO0_TYPE
{
    uint8_t d8;
    struct
    {
        uint8_t ota_valid: 1;               /* bit[0]: ota valid */
        uint8_t is_rom_code_patch: 1;       /* bit[1]: is rom code patch ? */
        uint8_t is_fw_trig_wdg_to: 1;       /* bit[2]: does fw trigger watchdog timeout ? */
        uint8_t is_airplane_mode: 1;        /* bit[3]: does h5 link reset ? */
        uint8_t is_send_patch_end_evt: 1;   /* bit[4]: does we send patch end event ? */
        uint8_t ota_mode: 1;                /* bit[5]: ota mode */
        uint8_t is_hci_mode: 1;             /* bit[6]: switch to hci mode? */
        uint8_t is_single_tone_mode: 1;                   /* bit[7]: reserved */
    };
} BTAON_FAST_REBOOT_SW_INFO0_TYPE;

/** @defgroup APP_OTA_Exported_Functions APP OTA service Exported Functions
    * @brief
    * @{
    */


/*============================================================================*
 *                              Private Functions
 *============================================================================*/
extern uint8_t btaon_fast_read_safe_8b(uint16_t offset);
extern void btaon_fast_write_safe_8b(uint16_t offset, uint8_t data);
/**
 * @brief  get 16bit data swapped.
 *
 * @param  val          16bit data to be swapped.
 * @return value after being swapped.
*/
static uint16_t swap_16(uint16_t val)
{
    uint16_t result;

    /* Idiom Recognition for REV16 */
    result = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);

    return result;
}

static bool dfu_check_sha256(T_IMG_HEADER_FORMAT *p_header)
{
    uint8_t sha256sum[SHA256_LENGTH];
    uint8_t sha256img[SHA256_LENGTH];
    uint8_t buf[SHA256_BUFFER_SIZE] = {0};
    uint32_t len;
    uint16_t i;
    uint16_t loop_cnt, remain_size;
    uint32_t pdata = (uint32_t)&p_header->ctrl_header;
    uint32_t s;

    len = sizeof(T_IMG_HEADER_FORMAT) - sizeof(T_AUTH_HEADER_FORMAT) +
          p_header->ctrl_header.payload_len;
    loop_cnt = len / SHA256_BUFFER_SIZE;
    remain_size = len % SHA256_BUFFER_SIZE;
    SHA256_CTX ctx = {0};
    SHA256_Init(&ctx);
    for (i = 0; i < loop_cnt; ++i)
    {
        s = os_lock();
        fmc_flash_nor_read(pdata, buf, SHA256_BUFFER_SIZE);
        os_unlock(s);
        if (i == 0)
        {
            //because not_ready is included when calculate image sha256
            ((T_IMG_CTRL_HEADER_FORMAT *)buf)->ctrl_flag.not_ready = 0;
        }
        SHA256_Update(&ctx, buf, SHA256_BUFFER_SIZE);
        pdata += SHA256_BUFFER_SIZE;
    }
    if (remain_size)
    {
        s = os_lock();
        fmc_flash_nor_read(pdata, buf, remain_size);
        os_unlock(s);
        if (loop_cnt == 0)
        {
            ((T_IMG_CTRL_HEADER_FORMAT *)buf)->ctrl_flag.not_ready = 0;  //for what?
        }

        SHA256_Update(&ctx, buf, remain_size);
    }
    SHA256_Final(&ctx, sha256sum);

    s = os_lock();
    fmc_flash_nor_read((uint32_t)&p_header->auth.image_hash, sha256img, SHA256_LENGTH);
    os_unlock(s);

    return (memcmp(sha256img, sha256sum, SHA256_LENGTH) == 0);
}

/*============================================================================*
 *                              Public Functions
 *============================================================================*/
/**
    * @brief  get image size of bank area
    * @param  image_id image ID
    * @return size
    */
uint32_t get_bank_size_by_img_id(IMG_ID image_id)
{
    uint32_t bank_size;

    T_IMG_HEADER_FORMAT *ota_header = (T_IMG_HEADER_FORMAT *)get_active_ota_bank_addr();
    bank_size = ota_header->image_info[(image_id - IMG_SBL) * 2 + 1];;

    return bank_size;
}


void dfu_fw_reboot(T_WDG_MODE reset_mode, T_RESET_REASON reason)
{
    chip_reset(reset_mode);
}

bool dfu_check_ota_mode_flag(void)
{
    BTAON_FAST_REBOOT_SW_INFO0_TYPE nFastBoot =
        (BTAON_FAST_REBOOT_SW_INFO0_TYPE)btaon_fast_read_safe_8b(BTAON_FAST_REBOOT_SW_INFO0);
    DFU_PRINT_INFO1("dfu_check_ota_mode_flag: ota(%d)", nFastBoot.ota_mode);

    return nFastBoot.ota_mode ? true : false;
}


void dfu_set_ota_mode_flag(bool enable)
{
    BTAON_FAST_REBOOT_SW_INFO0_TYPE nFastBoot = {.d8 = btaon_fast_read_safe_8b(BTAON_FAST_REBOOT_SW_INFO0)};

    if (enable)
    {
        nFastBoot.ota_mode = 1;
    }
    else
    {
        nFastBoot.ota_mode = 0;
    }
    btaon_fast_write_safe_8b(BTAON_FAST_REBOOT_SW_INFO0, nFastBoot.d8);
    DFU_PRINT_INFO1("dfu_set_ota_mode_flag ota(%d)", nFastBoot.ota_mode);
}

/**
 * @brief switch to the OTA mode, if support normal ota app need call it.
*/
void dfu_switch_to_ota_mode(void)
{
    DFU_PRINT_INFO0("[==>dfu_switch_to_ota_mode");
    dfu_set_ota_mode_flag(true);
    dfu_fw_reboot(RESET_ALL_EXCEPT_AON, DFU_SWITCH_TO_OTA_MODE);
}

/**
    * @brief    get the ic type of current fw
    * @param    void
    * @return   ic type
    */
uint8_t dfu_get_ic_type(void)
{
    uint8_t ic_type = 0x0B;
    uint32_t image_addr = get_header_addr_by_img_id(IMG_MCUAPP);

    ic_type = ((T_IMG_HEADER_FORMAT *)image_addr)->ctrl_header.ic_type;

    return ic_type;
}

/**
    * @brief    check if ota header exist
    * @param    header_addr    ota header address
    * @return   true: exist; false: not exist
    */
bool check_ota_header(uint32_t header_addr)
{
    if (header_addr == 0)
    {
        return false;
    }

    T_IMG_HEADER_FORMAT *header = (T_IMG_HEADER_FORMAT *)header_addr;

    if (header->magic_pattern != FLASH_TABLE_MAGIC_PATTERN
        || header->ctrl_header.image_id != IMG_OTA)
    {
        return false;
    }

    return true;
}

/**
    * @brief    get inactive bank's image address
    * @param    image_id   image id
    * @return   image address
    */
uint32_t get_temp_ota_bank_img_addr_by_img_id(IMG_ID image_id)
{
    uint32_t image_addr = 0;
    uint32_t temp_bank_addr;
    uint32_t ota_bank0_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0);
#if 0
    if ((USER_IMG_ID)image_id <= IMG_USER_DATA1 && (USER_IMG_ID)image_id > IMG_USER_DATA_MIN)
    {
        dfu_get_user_data_info((USER_IMG_ID)image_id, &image_addr, true);
        return image_addr;
    }
#endif
    if (image_id == PRE_IMG_VP)
    {
        const T_STORAGE_PARTITION_INFO *info = storage_partition_get(VP_PARTITION_NAME);
        return info->address;
    }

    if (image_id < IMG_OTA || image_id >= IMAGE_MAX)
    {
        return image_addr;
    }

    if (!is_ota_support_bank_switch())
    {
        if (image_id == IMG_OTA)
        {
            return 0;
        }

        image_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_TMP);
    }
    else
    {
        if (ota_bank0_addr == get_active_ota_bank_addr())
        {
            temp_bank_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_1);
        }
        else
        {
            temp_bank_addr = ota_bank0_addr;
        }

        if (image_id == IMG_OTA)
        {
            image_addr = temp_bank_addr;
        }
        else
        {
            if (!check_ota_header(temp_bank_addr))
            {
                return 0;
            }

            image_addr = ((T_IMG_HEADER_FORMAT *)temp_bank_addr)->image_info[(image_id - IMG_SBL) * 2];
        }
    }

    if (image_addr == 0xffffffff)
    {
        return 0;
    }

    return (image_addr | FLASH_OFFSET_TO_NO_CACHE);
}


/**
    * @brief    get inactive bank's image size
    * @param    image_id   image id
    * @return   image size
    */
uint32_t get_temp_ota_bank_img_size_by_img_id(IMG_ID image_id)
{
    uint32_t image_size = 0;
    uint32_t temp_bank_addr;
    uint32_t ota_bank0_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_0);

    if (image_id < IMG_OTA ||
        ((image_id >= IMAGE_MAX)
#if 0
         && ((USER_IMG_ID)image_id <= IMG_USER_DATA_MIN)
#endif
        ))
    {
        return image_size;
    }
#if 0
    if ((USER_IMG_ID)image_id > IMG_USER_DATA_MIN && (USER_IMG_ID)image_id <= IMG_USER_DATA1)
    {
        dfu_get_user_data_info((USER_IMG_ID)image_id, &image_size, false);
        return image_size;
    }
#endif
    /* IMG_OTA <= image_id < PRE_IMAGE_MAX, image in bank */
    if (!is_ota_support_bank_switch())
    {
        if (image_id == IMG_OTA)
        {
            return 0;
        }

        image_size = flash_partition_size_get(PARTITION_FLASH_OTA_TMP);
    }
    else
    {
        if (ota_bank0_addr == get_active_ota_bank_addr())
        {
            temp_bank_addr = flash_partition_addr_get(PARTITION_FLASH_OTA_BANK_1);
        }
        else
        {
            temp_bank_addr = ota_bank0_addr;
        }

        if (image_id == IMG_OTA)
        {
            image_size = OTA_HEADER_SIZE;
        }
        else
        {
            if (!check_ota_header(temp_bank_addr))
            {
                return 0;
            }

            image_size = ((T_IMG_HEADER_FORMAT *)temp_bank_addr)->image_info[(image_id - IMG_SBL) * 2 + 1];
        }
    }

    return image_size;
}

/**
* @brief calculate checksum of lenth of buffer in flash.
*
* @param  signature          signature to identify FW.
* @param  offset             offset of the image.
* @param  length             length of data.
* @param  crcValue          ret crc value point.
* @return  0 if buffer checksum calcs successfully, error line number otherwise
*/

uint32_t dfu_check_bufcrc(uint8_t *buf, uint32_t length, uint16_t mCrcVal)
{
    uint32_t ret = 0;
    uint16_t checksum16 = 0;
    uint32_t i;
    uint16_t *p16;

    p16 = (uint16_t *)buf;
    for (i = 0; i < length / 2; ++i)
    {
        checksum16 = checksum16 ^ (*p16);
        ++p16;
    }

    checksum16 = swap_16(checksum16);
    if (checksum16 != mCrcVal)
    {
        ret = __LINE__;
    }

    DFU_PRINT_TRACE2("<==dfu_check_bufcrc: checksum16=0x%x, mCrcVal=%x", checksum16, mCrcVal);

    return ret;
}

/**
 * @brief erase a sector of the flash.
 *
 * @param  signature          signature to identify FW.
 * @param  offset             offset of the image.
 * @return  0 if erase successfully, error line number otherwise

uint32_t dfu_flash_erase(IMG_ID image_id, uint32_t offset)
{
    uint32_t result = 0;
    uint32_t dfu_base_addr;

    dfu_base_addr = get_temp_ota_bank_img_addr_by_img_id(image_id);

    if (dfu_base_addr == 0)
    {
        return __LINE__;
    }

    result = fmc_flash_nor_erase(dfu_base_addr + offset, FMC_FLASH_NOR_ERASE_SECTOR);

    DFU_PRINT_TRACE1("<==dfu_flash_erase: result=%d", result);

    return result;
}
*/
/**
    * @brief    write data to flash
    * @param    img_id  image id
    * @param    offset  image offset
    * @param    total_offset  total offset when ota temp mode
    * @param    p_void  point of data
    * @return   0: success; other: fail
    */
uint32_t dfu_write_data_to_flash(uint16_t img_id, uint32_t offset, uint32_t total_offset,
                                 uint32_t length, void *p_void)
{
    uint32_t ret = 0;
    uint32_t dfu_base_addr;
    uint8_t readback_buffer[READ_BACK_BUFFER_SIZE];
    uint32_t read_back_len;
    uint32_t dest_addr;
    uint8_t *p_src = (uint8_t *)p_void;
    uint32_t remain_size = length;
    uint32_t s;

#if (DFU_COMMON_DEBUG == 1)
    DFU_PRINT_TRACE3("==>dfu_write_data_to_flash: total_offset=0x%x, offset=%d, length=%d",
                     total_offset, offset, length);
#endif

    if (p_void == 0)
    {
        ret = __LINE__;
        goto L_EXIT;
    }

    dfu_base_addr = get_temp_ota_bank_img_addr_by_img_id((IMG_ID)img_id);

    if (dfu_base_addr == 0)
    {
        ret = __LINE__;
        goto L_EXIT;
    }

    if (img_id >= IMG_OTA && img_id < IMAGE_MAX)
    {
        dfu_base_addr += total_offset;
    }

    /*cfu packet is 52 bytes. not ready bit offset in image header is 308 bytes*/
    if (offset == ((NOT_READY_OFFSET / CFU_PACKET_SIZE) * CFU_PACKET_SIZE))
    {
        uint8_t *not_ready = (uint8_t *)p_void + NOT_READY_OFFSET % CFU_PACKET_SIZE;
        *not_ready = (*not_ready) | 0x80;
        APP_PRINT_TRACE1("==>dfu_write_data_to_flash:set not ready bit. img_id 0x%x",
                         *((uint16_t *)((uint8_t *)p_void + NOT_READY_OFFSET % CFU_PACKET_SIZE + 2)));
    }

    dest_addr = dfu_base_addr + offset;
#if (DFU_COMMON_DEBUG == 1)
    DFU_PRINT_TRACE2("==>dfu_write_data_to_flash:dfu_base_addr=0x%x, dest_addr=0x%x", dfu_base_addr,
                     dest_addr);
#endif
    if ((dest_addr % FLASH_SECTOR_SIZE) == 0)
    {
        s = os_lock();
        fmc_flash_nor_erase(dest_addr, FMC_FLASH_NOR_ERASE_SECTOR);
        os_unlock(s);
    }
    else
    {
        if ((dest_addr / FLASH_SECTOR_SIZE) != ((dest_addr + length) / FLASH_SECTOR_SIZE))
        {
            if ((dest_addr + length) % FLASH_SECTOR_SIZE)
            {
                s = os_lock();
                fmc_flash_nor_erase((dest_addr + length) & ~(FLASH_SECTOR_SIZE - 1),
                                    FMC_FLASH_NOR_ERASE_SECTOR);
                os_unlock(s);
            }
        }
    }

    s = os_lock();
    fmc_flash_nor_write(dest_addr, p_void, length);
    os_unlock(s);

    SCB_InvalidateDCache_by_Addr((uint32_t *)dest_addr, length);

    while (remain_size)
    {
        read_back_len = (remain_size >= READ_BACK_BUFFER_SIZE) ? READ_BACK_BUFFER_SIZE : remain_size;
        s = os_lock();
        fmc_flash_nor_read(dest_addr, readback_buffer, read_back_len);
        os_unlock(s);

        if (memcmp(readback_buffer, p_src, read_back_len) != 0)
        {
            DFU_PRINT_TRACE2("==>dfu_write_data_to_flash, %d readback %b, p_src %b", TRACE_BINARY(read_back_len,
                             readback_buffer), TRACE_BINARY(read_back_len, p_src));
            ret = __LINE__;
            goto L_EXIT;
        }

        dest_addr += read_back_len;
        p_src += read_back_len;
        remain_size -= read_back_len;
    }

L_EXIT:
    DFU_PRINT_TRACE1("<==dfu_write_data_to_flash: ret=%d", ret);
    return ret;
}


/**
    * @brief    check the integrity of the image
    * @param    img_id    image id
    * @param    offset  address offset
    * @return   ture:success ; false: fail
    */
bool dfu_checksum(IMG_ID img_id, uint32_t offset)
{
    uint32_t base_addr = 0;
    bool ret = false;
    DFU_PRINT_TRACE2("==>dfu_checksum: image_id=0x%x, offset=%d", img_id, offset);

    base_addr = get_temp_ota_bank_img_addr_by_img_id(img_id);

    if (base_addr == 0)
    {
        return false;
    }

    if (img_id >= IMG_OTA && img_id < IMAGE_MAX)
    {
        base_addr += offset;
    }


    ret = dfu_check_sha256((T_IMG_HEADER_FORMAT *)base_addr);

    if (ret == true && !is_ota_support_bank_switch())
    {
#if 0
        /* only set userdata image ready when valid fw and not support bank switch*/
        if (img_id > IMG_USER_DATA_MIN && img_id <= IMG_USER_DATA1)
        {
            //user data no temp bank, needn't cal offset
            dfu_set_ready((T_IMG_HEADER_FORMAT *)base_addr);
        }
#endif
    }


    DFU_PRINT_TRACE2("<==dfu_check_checksum: base_addr=0x%x, ret=%d", base_addr, ret);

    return ret;
}

/**
    * @brief    clear not ready flag of specific image
    * @param    addr    address of the image
    * @return   void
    */
void dfu_set_ready(T_IMG_HEADER_FORMAT *p_header)
{
    T_IMG_CTRL_HEADER_FORMAT ctrl_header;
    uint16_t ctrl_flag;
    uint32_t s;

    s = os_lock();
    fmc_flash_nor_read((uint32_t) & (p_header->ctrl_header.ctrl_flag), (uint8_t *)&ctrl_flag,
                       sizeof(ctrl_flag));
    os_unlock(s);
    DFU_PRINT_TRACE2("==>dfu_set_ready: p_header 0x%x, ctrl_flag 0x%x", p_header, ctrl_flag);

    ctrl_flag &= ~0x80;
    s = os_lock();
    fmc_flash_nor_write((uint32_t) & (p_header->ctrl_header.ctrl_flag), (uint8_t *)&ctrl_flag,
                        sizeof(ctrl_flag));
    fmc_flash_nor_read((uint32_t)&p_header->ctrl_header, (uint8_t *)&ctrl_header, sizeof(ctrl_header));
    os_unlock(s);
    DFU_PRINT_TRACE2("<==dfu_set_ready:img_id 0x%x, after ctrl_flag 0x%x", ctrl_header.image_id,
                     ctrl_header.ctrl_flag);
}

/**
    * @brief    clear not obsolete flag of specific image
    * @param    addr    address of the image
    * @return   void
    */
void dfu_set_obsolete(T_IMG_HEADER_FORMAT *p_header)
{
    uint16_t ctrl_flag;
    uint32_t s;
    s = os_lock();
    fmc_flash_nor_read((uint32_t) & (p_header->ctrl_header.ctrl_flag), (uint8_t *)&ctrl_flag,
                       sizeof(ctrl_flag));
    ctrl_flag &= ~0x0100;
    fmc_flash_nor_write((uint32_t) & (p_header->ctrl_header.ctrl_flag), (uint8_t *)&ctrl_flag,
                        sizeof(ctrl_flag));
    os_unlock(s);
}

#if 0
T_USER_DATA_ERROR_TYPE dfu_get_user_data_info(USER_IMG_ID image_id,
                                              uint32_t *img_info, bool is_addr)
{
    T_USER_DATA_ERROR_TYPE err_code = USER_DATA_SUCCESS;

    switch (image_id)
    {
    case IMG_USER_DATA1:
        if (USER_DATA1_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA1_ADDR : USER_DATA1_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA2:
        if (USER_DATA2_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA2_ADDR : USER_DATA2_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA3:
        if (USER_DATA3_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA3_ADDR : USER_DATA3_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA4:
        if (USER_DATA4_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA4_ADDR : USER_DATA4_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA5:
        if (USER_DATA5_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA5_ADDR : USER_DATA5_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA6:
        if (USER_DATA6_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA6_ADDR : USER_DATA6_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA7:
        if (USER_DATA7_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA7_ADDR : USER_DATA7_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    case IMG_USER_DATA8:
        if (USER_DATA8_WITH_HEADER)
        {
            *img_info = is_addr ? USER_DATA8_ADDR : USER_DATA8_SIZE;
        }
        else
        {
            err_code = USER_DATA_NOT_SUPPORT_OTA;
            *img_info = 0;
        }
        break;
    default:
        APP_PRINT_ERROR1("error user data image_id 0x%x", image_id);
        err_code = USER_DATA_TYPE_ERROR;
        *img_info = 0;
        break;
    }

    if (is_addr && (*img_info))
    {
        uint32_t addr = *img_info;
        *img_info = FMC_MAIN0_NON_CACHE_ADDR(addr);
    }

    return err_code;
}
#endif
/** End of APP_OTA_Exported_Functions
    * @}
    */

/** @} */ /* End of group APP_OTA_SERVICE */
