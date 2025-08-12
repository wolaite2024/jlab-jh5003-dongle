/**
************************************************************************************************************
*               Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file     platform_utils.h
* @brief    utility helper function for user application
* @author   lory_xu
* @date     2017-02
* @version  v1.0
*************************************************************************************************************
*/

#ifndef _PLATFORM_UTILS_H_
#define _PLATFORM_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

typedef struct _RTLNUM_Type
{
    uint16_t rtl_num1;
    uint16_t rtl_num2;
} RTLNUM_Type;

/** @defgroup  PLATFORM_UTILS Platform Utilities
    * @brief Utility helper functions
    * @{
    */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup PLATFORM_UTILS_Exported_Functions Platform Utils Exported Functions
    * @brief
    * @{
    */

/**
 * @brief Generate random number given max number allowed
 * @param max   to specify max number that allowed
 * @return random number
 */

uint32_t platform_random(uint32_t max);

/**
 * @brief Busy delay for specified millisecond
 * @param t   to specify t milliseconds to delay
 * @return none
 */
extern void (*platform_delay_ms)(uint32_t t);

/**
 * @brief Busy delay for specified micro second
 * @param t   to specify t micro seconds to delay
 * @return none
 */
extern void (*platform_delay_us)(uint32_t t);

extern uint32_t (*read_cpu_counter)(void);

extern RTLNUM_Type(*get_soc_rtl_num)(void);

extern uint32_t loop_cycle_count;

int32_t saturate_by_boundary(int32_t value, int32_t upperbound, int32_t lowerbound);

bool get_disable_hci_bt_test(void);
bool get_airplane_mode(void);
void set_airplane_mode(bool airplane_mode);
bool get_timer_clk_src_from_40m(void);
uint8_t get_default_hci_rx_pin(void);
uint8_t get_default_hci_tx_pin(void);
void cache_restore(bool i_cache_en, bool d_cache_en);
void set_cpu_loop_buffer_rom(bool is_enable);

extern bool (*patch_platform_assert_dump)(const char *pFuncName, uint32_t funcLine,
                                          uint32_t *assert_args);

extern uint32_t (*get_occd_addr)(void);
extern uint32_t (*get_occd_size)(void);
extern uint32_t (*get_extra_occd_addr)(void);
extern uint32_t (*get_extra_occd_size)(void);

extern void (*set_cpu_loop_buffer)(bool is_enable);

#define SYS_VENDOR_CNT_BIT_NUM                BIT64(32)

#define get_disable_hci_mac_rf_access()       false
#define get_disable_hci_wifi_coexist_func()   false
#define get_disable_hci_set_uart_baudrate()   false
#define get_disable_hci_rf_dbg_func()         false
#define get_disable_hci_read_chip_info()      false
#define get_disable_hci_bt_extension()        false
#define get_disable_hci_bt_dbg_func()         false
#define get_disable_hci_rf_test()             false

/** @} */ /* End of group PLATFORM_UTILS_Exported_Functions */

/** @} */ /* End of group PLATFORM_UTILS */

#ifdef __cplusplus
}
#endif

#endif

