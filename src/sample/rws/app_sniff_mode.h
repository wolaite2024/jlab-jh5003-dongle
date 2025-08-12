/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _APP_SNIFF_MODE_H_
#define _APP_SNIFF_MODE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum
{
    SNIFF_DISABLE_MASK_ACL               = 0x00000001,
    SNIFF_DISABLE_MASK_A2DP              = 0x00000002,
    SNIFF_DISABLE_MASK_SCO               = 0x00000004,
    SNIFF_DISABLE_MASK_VA                = 0x00000008,
    SNIFF_DISABLE_MASK_OTA               = 0x00000010,
    SNIFF_DISABLE_MASK_ANC_APT           = 0x00000020,
    SNIFF_DISABLE_MASK_LINKBACK          = 0x00000040,
    SNIFF_DISABLE_MASK_LP                = 0x00000080,
    SNIFF_DISABLE_MASK_TRANS             = 0x00000100,
    SNIFF_DISABLE_MASK_ROLESWITCH        = 0x00000200,
    SNIFF_DISABLE_MASK_GAMINGMODE_DONGLE = 0x00000400,
    SNIFF_DISABLE_MASK_SPP_CAPTURE       = 0x00000800,
    SNIFF_DISABLE_MASK_LEA               = 0x00001000,
    SNIFF_DISABLE_MASK_SPP_RECORD        = 0x00002000,
    SNIFF_DISABLE_MASK_DUT_TEST_MODE     = 0x00004000,
    SNIFF_DISABLE_MASK_SPP_CTRL_PKT      = 0x00008000,
    SNIFF_DISABLE_MASK_FIX_CHAN_DATA     = 0x00010000,
} T_SNIFF_DISABLE_MASK;

/**
    * @brief  app_sniff_mode_startup
    * @param  void
    * @return void
    */
void app_sniff_mode_startup(void);

/**
    * @brief  app_sniff_mode_b2s_disable
    * @param  bd_addr the bluetooth address
    * @param  flag the sniff mode flag
    * @return void
    */
void app_sniff_mode_b2s_disable(uint8_t *bd_addr, uint32_t flag);

/**
    * @brief  app_sniff_mode_b2s_enable
    * @param  bd_addr the bluetooth address
    * @param  flag the sniff mode flag
    * @return void
    */
void app_sniff_mode_b2s_enable(uint8_t *bd_addr, uint32_t flag);

/**
    * @brief  app_sniff_mode_b2s_disable_all
    * @param  bd_addr the bluetooth address
    * @param  flag the sniff mode flag
    * @return void
    */
void app_sniff_mode_b2s_disable_all(uint32_t flag);

/**
    * @brief  app_sniff_mode_b2s_enable_all
    * @param  flag the sniff mode flag
    * @return void
    */
void app_sniff_mode_b2s_enable_all(uint32_t flag);

/**
    * @brief  app_sniff_mode_b2s_check_left_flag_when_disconnect
    * @param  bd_addr the bluetooth address
    * @return void
    */
void app_sniff_mode_b2s_check_left_flag_when_disconnect(uint8_t *bd_addr);

/**
    * @brief  app_sniff_mode_b2b_disable
    * @param  bd_addr the bluetooth address
    * @param  flag the sniff mode flag
    * @return void
    */
void app_sniff_mode_b2b_disable(uint8_t *bd_addr, uint32_t flag);

/**
    * @brief  app_sniff_mode_b2b_enable
    * @param  bd_addr the bluetooth address
    * @param  flag the sniff mode flag
    * @return void
    */
void app_sniff_mode_b2b_enable(uint8_t *bd_addr, uint32_t flag);

/**
    * @brief  app_sniff_mode_disable_all
    * @param  void
    * @return void
    */
void app_sniff_mode_disable_all(void);

/**
    * @brief  app_sniff_mode_roleswap_suc
    * @param  void
    * @return void
    */
void app_sniff_mode_roleswap_suc(void);

/**
    * @brief  app_sniff_mode_send_roleswap_info
    * @param  void
    * @return void
    */
void app_sniff_mode_send_roleswap_info(void);

/**
    * @brief  app_sniff_mode_recv_roleswap_info
    * @param  buf  buffer of the sync info
    * @param  len  length of the sync info
    * @return void
    */
void app_sniff_mode_recv_roleswap_info(void *buf, uint16_t len);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _APP_SNIFF_MODE_H_ */
