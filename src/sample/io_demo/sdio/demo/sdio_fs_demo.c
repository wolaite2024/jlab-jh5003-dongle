/**
*********************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     sdio_fs_demo.c
* @brief    This file provides SDIO demo code.
* @details  Please create a folder named BBPro in root directory.
* @author   elliot chen
* @date     2017-1-16
* @version  v1.0
*********************************************************************************************************
*/

/* Includes -----------------------------------------------------------------*/
#include <string.h>
#include <wchar.h>
#include "sd.h"
#include "ff.h"
#include "trace.h"
#include "rtl876x.h"

/** @defgroup  SDIO_FS_DEMO  SDIO FS DEMO
    * @brief
    * @{
    */


/* Defines ------------------------------------------------------------------*/
static const T_SD_CONFIG    sd_card_cfg =
{
    .sd_if_type = SD_IF_SD_CARD,
    .sdh_group = GROUP_0,
    .sdh_bus_width = SD_BUS_WIDTH_4B,
    .sd_bus_clk_sel = SD_BUS_CLK_20M,
#if defined (TARGET_RTL87X3E)
    .sd_power_en = 1,
    .sd_power_high_active = 1,
    .sd_power_pin = P5_6
#endif
};

/* Globals ------------------------------------------------------------------*/

/* FatFS */
FATFS fs;
/* file objects */
FIL fsrc, fdst;
/* file copy buffer */
char buffer[1024];
DIR dir;
FILINFO fileinfo;

/**
  * @brief  initialization of pinmux settings and pad settings.
  * @param   No parameter.
  * @return  void
  */
void Board_SD_Init(void)
{
    sd_config_init((T_SD_CONFIG *)&sd_card_cfg);
    sd_board_init();
}

/**
  * @brief  The function is to write and read file.
  * @param   No parameter.
  * @return  void
  */
void FatFS_Demo(void)
{
    uint32_t a = 1;
    uint32_t res = 0;
    const TCHAR *driver_num = (const TCHAR *)_T("0:");

    /* Create workspace */
    res = f_mount(&fs, driver_num, 1);
    if (res != 0)
    {
        IO_PRINT_ERROR1("FatFS_Demo: f_mount fail, res %d", res);
        return;
    }

    /* Open file */
    res = f_open(&fdst, (const TCHAR *)_T("Data.txt"), FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        //Open file failure, add error handle code here
        f_close(&fdst);
        return ;
    }

    /* File operation */
    for (uint32_t i = 0; i < 1024; i++)
    {
        buffer[i] = i % 0x09 + 0x30;
    }
    buffer[1021] = 0x65; //e
    buffer[1022] = 0x6E; //n
    buffer[1023] = 0x64; //d
    f_write(&fdst, buffer, 1024, &a);

    /* Close file */
    f_close(&fdst);


    res = f_open(&fsrc, (const TCHAR *)_T("Data.txt"), FA_OPEN_EXISTING | FA_READ);
    if (res != FR_OK)
    {
        //Open file failure, add error handle code here
        f_close(&fsrc);
        return ;
    }

    memset(buffer, 0, 1024);
    f_read(&fsrc, buffer, 1024, &a);

    IO_PRINT_INFO1("FatFS_Demo: read_len %d", a);
    sd_print_binary_data((uint8_t *)buffer, 1024);

    /* Close file */
    f_close(&fsrc);
}

/**
  * @brief  The function is to write all files in the BBPro directory.
  * @param   No parameter.
  * @return  void
  */
void FatFS_DirectoyDemo(void)
{
    uint32_t a = 1;
    const TCHAR *driver_num = (const TCHAR *)_T("0:");
    wchar_t path[50] = _T("0:/BBPro/");
    uint16_t *pname = path;
    FRESULT res;

    /* Create workspace */
    res = f_mount(&fs, driver_num, 1);
    if (res != 0)
    {
        IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_mount fail, res %d", res);
        return;
    }

    /* change directory */
    res = f_chdir(path);
    if ((res != FR_OK) && (res != FR_NO_PATH))
    {
        IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_chdir fail, res %d", res);
        return;
    }
    else if (res == FR_NO_PATH)
    {
        /* create directory based on path */
        res = f_mkdir(path);
        if (res == FR_OK)
        {
            res = f_chdir(path);
            if (res != FR_OK)
            {
                IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_chdir fail after mkdir, res %d", res);
                return;
            }
        }
        else
        {
            IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_mkdir fail, res %d", res);
            return;
        }
    }

    res = f_open(&fdst, (const TCHAR *)_T("0:/BBPro/test.txt"), FA_CREATE_ALWAYS | FA_WRITE);
    if (res != 0)
    {
        IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_open fail, res %d", res);
        f_close(&fdst);
        return;
    }
    f_close(&fdst);

    /* Open directory */
    res = f_opendir(&dir, path);
    if (res != FR_OK)
    {
        IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_opendir fail, res %d", res);
        return;
        //Open directory failure, add error handle code here
    }

    while (1)
    {
        res = f_readdir(&dir, &fileinfo);

        /* Traverse all files */
        if ((res != FR_OK) || (fileinfo.fname[0] == 0))
        {
            IO_PRINT_INFO1("FatFS_DirectoyDemo: exit, res %d", res);
            break;
        }

        for (uint32_t i = 0; i < 13; i++)
        {
            //IO_PRINT_INFO2("fileinfo.fname[%d] = 0x%x ", i, fileinfo.fname[i]);
            *(pname + i + 9) = fileinfo.fname[i];
        }

        /* Open file */
        res = f_open(&fdst, (const TCHAR *)pname, FA_OPEN_EXISTING | FA_WRITE);
        if (res != FR_OK)
        {
            //Open file failure, add error handle code here
            IO_PRINT_ERROR1("FatFS_DirectoyDemo: f_open fail, res %d", res);
            f_close(&fdst);
            return ;
        }

        /* File operation */
        for (uint32_t i = 0; i < 1024; i++)
        {
            buffer[i] = i % 0x09 + 0x30;
        }
        buffer[1021] = 'e';
        buffer[1022] = 'n';
        buffer[1023] = 'd';
        res = f_write(&fdst, buffer, 1024, &a);
        if (res == FR_OK)
        {
            IO_PRINT_INFO0("FatFS_DirectoyDemo: write OK");
        }

        /* Close file */
        f_close(&fdst);
    }
}

/**
  * @brief  demo code of SDIO communication.
  * @param   No parameter.
  * @return  void
  */
void SD_DemoCode(void)
{
    /* PAD configure */
    Board_SD_Init();

    /* Demo code */
    FatFS_DirectoyDemo();
    //FatFS_Demo();
}

/** @} */ /* End of group SDIO_FS_DEMO */

/******************* (C) COPYRIGHT 2017 Realtek Semiconductor Corporation *****END OF FILE****/

