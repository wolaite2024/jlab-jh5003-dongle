/** Copyright(c) 2018, Realtek Semiconductor Corporation.All rights reserved.*/

#ifndef _FLASH_MAP_H_
#define _FLASH_MAP_H_

#define IMG_HDR_SIZE                    0x00000400  //Changable, default 4K, modify in eFuse if needed

#define CFG_FILE_PAYLOAD_LEN            0x00001000  //Fixed
#define FLASH_SIZE                      0x00400000  //4096K Bytes


/* ========== High Level Flash Layout Configuration ========== */
#define EQ_FITTING_ADDR                 0x02000000
#define EQ_FITTING_SIZE                 0x00000400  //1K Bytes
#define RSV_ADDR                        0x02000400
#define RSV_SIZE                        0x00001C00  //7K Bytes
#define OEM_CFG_ADDR                    0x02002000
#define OEM_CFG_SIZE                    0x00001400  //5K Bytes
#define BOOT_PATCH_ADDR                 0x02004000
#define BOOT_PATCH_SIZE                 0x00002000  //8K Bytes
#define PLATFORM_EXT_ADDR               0x02006000
#define PLATFORM_EXT_SIZE               0x00008000  //32K Bytes
#define LOWERSTACK_EXT_ADDR             0x0200E000
#define LOWERSTACK_EXT_SIZE             0x00007000  //28K Bytes
#define UPPERSTACK_ADDR                 0x02015000
#define UPPERSTACK_SIZE                 0x0003F000  //252K Bytes
#define OTA_BANK0_ADDR                  0x02054000
#define OTA_BANK0_SIZE                  0x001AC000  //1712K Bytes
#define OTA_BANK1_ADDR                  0x02200000
#define OTA_BANK1_SIZE                  0x001AC000  //1712K Bytes
#define VP_DATA_ADDR                    0x023AC000
#define VP_DATA_SIZE                    0x00032000  //200K Bytes
#define FTL_ADDR                        0x023DE000
#define FTL_SIZE                        0x00015000  //84K Bytes
#define BKP_DATA1_ADDR                  0x00000000
#define BKP_DATA1_SIZE                  0x00000000  //0K Bytes
#define BKP_DATA2_ADDR                  0x023FE000
#define BKP_DATA2_SIZE                  0x00002000  //8K Bytes
#define OTA_TMP_ADDR                    0x00000000
#define OTA_TMP_SIZE                    0x00000000  //0K Bytes

/* ========== OTA Bank0 Flash Layout Configuration ========== */
#define BANK0_OTA_HDR_ADDR              0x02054000
#define BANK0_OTA_HDR_SIZE              0x00000400  //1K Bytes
#define BANK0_FSBL_ADDR                 0x02054400
#define BANK0_FSBL_SIZE                 0x00001C00  //7K Bytes
#define BANK0_STACK_PATCH_ADDR          0x02056000
#define BANK0_STACK_PATCH_SIZE          0x0003D000  //244K Bytes
#define BANK0_SYS_PATCH_ADDR            0x02093000
#define BANK0_SYS_PATCH_SIZE            0x0001D000  //116K Bytes
#define BANK0_APP_ADDR                  0x020B0000
#define BANK0_APP_SIZE                  0x000DE000  //888K Bytes
#define BANK0_DSP_SYS_ADDR              0x0218E000
#define BANK0_DSP_SYS_SIZE              0x00020000  //128K Bytes
#define BANK0_DSP_APP_ADDR              0x021AE000
#define BANK0_DSP_APP_SIZE              0x00038000  //224K Bytes
#define BANK0_DSP_CFG_ADDR              0x021E6000
#define BANK0_DSP_CFG_SIZE              0x0000A000  //40K Bytes
#define BANK0_APP_CFG_ADDR              0x021F0000
#define BANK0_APP_CFG_SIZE              0x00002000  //8K Bytes
#define BANK0_EXT_IMG0_ADDR             0x021F2000
#define BANK0_EXT_IMG0_SIZE             0x0000A000  //40K Bytes
#define BANK0_EXT_IMG1_ADDR             0x021FC000
#define BANK0_EXT_IMG1_SIZE             0x00004000  //16K Bytes
#define BANK0_EXT_IMG2_ADDR             0x00000000
#define BANK0_EXT_IMG2_SIZE             0x00000000  //0K Bytes
#define BANK0_EXT_IMG3_ADDR             0x00000000
#define BANK0_EXT_IMG3_SIZE             0x00000000  //0K Bytes

/* ========== OTA Bank1 Flash Layout Configuration ========== */
#define BANK1_OTA_HDR_ADDR              0x02200000
#define BANK1_OTA_HDR_SIZE              0x00000400  //1K Bytes
#define BANK1_FSBL_ADDR                 0x02200400
#define BANK1_FSBL_SIZE                 0x00001C00  //7K Bytes
#define BANK1_STACK_PATCH_ADDR          0x02202000
#define BANK1_STACK_PATCH_SIZE          0x0003D000  //244K Bytes
#define BANK1_SYS_PATCH_ADDR            0x0223F000
#define BANK1_SYS_PATCH_SIZE            0x0001D000  //116K Bytes
#define BANK1_APP_ADDR                  0x0225C000
#define BANK1_APP_SIZE                  0x000DE000  //888K Bytes
#define BANK1_DSP_SYS_ADDR              0x0233A000
#define BANK1_DSP_SYS_SIZE              0x00020000  //128K Bytes
#define BANK1_DSP_APP_ADDR              0x0235A000
#define BANK1_DSP_APP_SIZE              0x00038000  //224K Bytes
#define BANK1_DSP_CFG_ADDR              0x02392000
#define BANK1_DSP_CFG_SIZE              0x0000A000  //40K Bytes
#define BANK1_APP_CFG_ADDR              0x0239C000
#define BANK1_APP_CFG_SIZE              0x00002000  //8K Bytes
#define BANK1_EXT_IMG0_ADDR             0x0239E000
#define BANK1_EXT_IMG0_SIZE             0x0000A000  //40K Bytes
#define BANK1_EXT_IMG1_ADDR             0x023A8000
#define BANK1_EXT_IMG1_SIZE             0x00004000  //16K Bytes
#define BANK1_EXT_IMG2_ADDR             0x00000000
#define BANK1_EXT_IMG2_SIZE             0x00000000  //0K Bytes
#define BANK1_EXT_IMG3_ADDR             0x00000000
#define BANK1_EXT_IMG3_SIZE             0x00000000  //0K Bytes


#endif /* _FLASH_MAP_H_ */
