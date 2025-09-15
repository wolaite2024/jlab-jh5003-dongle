/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <string.h>
#include "diskio.h"     /* Declarations of disk functions */
#include "os_mem.h"
#include "trace.h"
#include "sd.h"

#define FATFS_SECTORE_SIZE     FF_MAX_SS
/* bb2: sdio can't access S1:0x00200000~0x002FFFFF */
#define SD_READ_BUF_LEN        (2 * FATFS_SECTORE_SIZE)
#define SD_WRITE_BUF_LEN       (2 * FATFS_SECTORE_SIZE)

static T_SD_STATE g_sd_cur_state = SD_POWER_OFF;

static void disk_enter_dlps(void)
{
    if (g_sd_cur_state != SD_POWER_OFF)
    {
        sd_deinit();
        g_sd_cur_state = SD_POWER_OFF;
    }
}

static void disk_exit_dlps(void)
{
    if (g_sd_cur_state == SD_POWER_OFF)
    {
        sd_init();
        g_sd_cur_state = SD_POWER_ON;
    }
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat = 0;
    T_SD_STATUS sdStatus = SD_OK;
    uint32_t cardStatus = 0;

    switch (pdrv)
    {
    case DEV_SD :
        {
            disk_exit_dlps();
            sdStatus = sd_check_program_status();
            if (sdStatus != SD_OK)
            {
                APP_PRINT_INFO2("reinit once: disk_status:0x%x, cardStatus:0x%x", sdStatus, cardStatus);
                stat = disk_initialize(pdrv);
            }
        }
        break;
    }
    return stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    switch (pdrv)
    {
    case DEV_SD :
        {
            disk_exit_dlps();
        }
        break;
    }
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    DRESULT res = RES_OK;
    T_SD_STATUS status = SD_OK;

    switch (pdrv)
    {
    case DEV_SD:
        {
            disk_exit_dlps();

            uint16_t remain_count = count;
            uint16_t read_offset = 0;
            uint32_t *pReadBuf = NULL;

            pReadBuf = os_mem_alloc(RAM_TYPE_BUFFER_ON, SD_READ_BUF_LEN);
            if (pReadBuf == NULL)
            {
                return RES_MALLOC;
            }

            for (; remain_count > 0;)
            {
                memset(pReadBuf, 0, SD_READ_BUF_LEN);
                if (remain_count > (SD_READ_BUF_LEN / FATFS_SECTORE_SIZE))
                {
                    status = sd_read(sector + read_offset / FATFS_SECTORE_SIZE, (uint32_t)pReadBuf,
                                     FATFS_SECTORE_SIZE, SD_READ_BUF_LEN / FATFS_SECTORE_SIZE);

//                    APP_PRINT_INFO4("disk_read 1: status:%d, sector:%d, count:%d, pReadBuf(%b))",
//                            status, sector + read_offset / FATFS_SECTORE_SIZE, SD_READ_BUF_LEN/FATFS_SECTORE_SIZE, TRACE_BINARY(10, (uint8_t *)pReadBuf));
                    if (status == SD_OK)
                    {
                        res = RES_OK;
                        memcpy((uint8_t *)buff + read_offset, (uint8_t *)pReadBuf, SD_READ_BUF_LEN);
                        remain_count -= SD_READ_BUF_LEN / FATFS_SECTORE_SIZE;
                        read_offset += SD_READ_BUF_LEN;
                    }
                    else
                    {
                        APP_PRINT_INFO2("disk_read 1: read fail, status:%d, count:%d", status, count);
                        res = RES_ERROR;
                        break;
                    }
                }
                else
                {
                    status = sd_read(sector + read_offset / FATFS_SECTORE_SIZE, (uint32_t)pReadBuf,
                                     FATFS_SECTORE_SIZE, remain_count);

//                    APP_PRINT_INFO4("disk_read 2: status:%d, sector:%d, count:%d, pReadBuf(%b))",
//                            status, sector + read_offset / FATFS_SECTORE_SIZE, remain_count, TRACE_BINARY(10, (uint8_t *)pReadBuf));
                    if (status == SD_OK)
                    {
                        res = RES_OK;
                        memcpy((uint8_t *)buff + read_offset, (uint8_t *)pReadBuf, (FATFS_SECTORE_SIZE * remain_count));
                        read_offset += FATFS_SECTORE_SIZE * remain_count;
                        remain_count = 0;
                    }
                    else
                    {
                        APP_PRINT_INFO2("disk_read 2: read fail, status:%d, count:%d", status, count);
                        res = RES_ERROR;
                        break;
                    }
                }
            }
//            APP_PRINT_INFO4("disk_read: status:%d, sector:%d, count:%d, pReadBuf(%b))",
//                            status, sector, count, TRACE_BINARY(10, (uint8_t *)pReadBuf));
            os_mem_free(pReadBuf);
            pReadBuf = NULL;
        }
        break;
    }
    return res;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    DRESULT res = RES_OK;
    T_SD_STATUS status = SD_OK;

    switch (pdrv)
    {
    case DEV_SD:
        {
            disk_exit_dlps();

            uint16_t remain_count = count;
            uint16_t read_offset = 0;
            uint32_t *pWriteBuf = NULL;

            pWriteBuf = os_mem_alloc(RAM_TYPE_BUFFER_ON, SD_WRITE_BUF_LEN);
            if (pWriteBuf == NULL)
            {
                return RES_MALLOC;
            }

            for (; remain_count > 0;)
            {
                if (remain_count > (SD_WRITE_BUF_LEN / FATFS_SECTORE_SIZE))
                {
                    memcpy((uint8_t *)pWriteBuf, (uint8_t *)buff + read_offset, SD_WRITE_BUF_LEN);
                    status = sd_write(sector + read_offset / FATFS_SECTORE_SIZE, (uint32_t)pWriteBuf,
                                      FATFS_SECTORE_SIZE, SD_WRITE_BUF_LEN / FATFS_SECTORE_SIZE);

//                    APP_PRINT_INFO4("disk_write 1: status:%d, sector:%d, count:%d, pWriteBuf(%b))",
//                            status, sector + read_offset / FATFS_SECTORE_SIZE, SD_READ_BUF_LEN/FATFS_SECTORE_SIZE, TRACE_BINARY(10, (uint8_t *)pWriteBuf));
                    if (status == SD_OK)
                    {
                        res = RES_OK;
                        remain_count -= SD_WRITE_BUF_LEN / FATFS_SECTORE_SIZE;
                        read_offset += SD_WRITE_BUF_LEN;
                    }
                    else
                    {
                        APP_PRINT_INFO2("disk_write 1: write fail, status:%d, count:%d", status, count);
                        res = RES_PARERR;
                        break;
                    }
                }
                else
                {
                    memcpy((uint8_t *)pWriteBuf, (uint8_t *)buff + read_offset, (FATFS_SECTORE_SIZE * remain_count));
                    status = sd_write(sector + read_offset / FATFS_SECTORE_SIZE, (uint32_t)pWriteBuf,
                                      FATFS_SECTORE_SIZE, remain_count);

//                    APP_PRINT_INFO4("disk_write 2: status:%d, sector:%d, count:%d, pWriteBuf(%b))",
//                            status, sector + read_offset / FATFS_SECTORE_SIZE, remain_count, TRACE_BINARY(10, (uint8_t *)pWriteBuf));
                    if (status == SD_OK)
                    {
                        res = RES_OK;
                        read_offset += FATFS_SECTORE_SIZE * remain_count;
                        remain_count = 0;
                    }
                    else
                    {
                        APP_PRINT_INFO2("disk_write 2: write fail, status:%d, count:%d", status, count);
                        res = RES_PARERR;
                        break;
                    }
                }
            }

//            APP_PRINT_INFO4("disk_write: status:%d, sector:%d, count:%d, pWriteBuf(%b))",
//                            status, sector, count, TRACE_BINARY(10, (uint8_t *)pWriteBuf));
            os_mem_free(pWriteBuf);
            pWriteBuf = NULL;
        }
        break;
    }
    return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff
)
{
    DRESULT res = RES_OK;

    switch (pdrv)
    {
    case DEV_SD:
        {
            switch (cmd)
            {
#if (FF_MAX_SS != FF_MIN_SS)
            /* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
            case GET_SECTOR_SIZE :
                {
                    *(WORD *)buff = FATFS_SECTORE_SIZE  /*SD_BLOCKSIZE*/;
                }
                break;
#endif

#if (FF_USE_MKFS == 1)
            /* Get erase block size (needed at FF_USE_MKFS == 1) */
            case GET_BLOCK_SIZE :
                {
                    *(DWORD *)buff = sd_get_dev_block_size();
                }
                break;

            case GET_SECTOR_COUNT:
                {
                    *(DWORD *)buff = sd_get_dev_capacity() / FATFS_SECTORE_SIZE;
                }
                break;
#endif

#if (FF_FS_READONLY == 0)
            /* Complete pending write process (needed at FF_FS_READONLY == 0) */
            case CTRL_SYNC :
                break;
#endif

#if (FF_USE_TRIM == 1)
            /* Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) */
            case CTRL_TRIM:
                {
                    SD_Status sdStatus = SD_OK;
                    LBA_t addr[2] = {0};
                    memcpy(addr, buff, sizeof(addr));
                    res = sd_erase((uint32_t)addr[0], (uint32_t)addr[1]);
                    if (sdStatus != FR_OK)
                    {
                        res = RES_ERROR;
                    }
                }
                break;
#endif
            case CTRL_POWER_ON: /* power on sd */
                {
                    disk_exit_dlps();
                }
                break;

            case CTRL_POWER_OFF:
                {
                    disk_enter_dlps();
                }
                break;

            case MMC_GET_SDSTAT:
                {
                    if (disk_status(pdrv) != 0)
                    {
                        res = RES_NOTRDY;
                    }
                }
                break;
            }
        }
        break;

    default:
        res = RES_PARERR;
        break;
    }
    return res;
}

DWORD get_fattime(void)
{
    return 0;
}
