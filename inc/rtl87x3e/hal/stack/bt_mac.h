#ifndef BT_MAC_H_
#define BT_MAC_H_

#include <stdint.h>
#include <stdbool.h>

#if defined (__cplusplus)
extern "C" {
#endif

/** @defgroup  HAL_87x3E_BT_MAC    BT MAC API
    * @brief BT MAC API
    * @{
    */

/**
 * @brief Read bluetooth piconet clock for application
 * @param conn_handle   identify which connection to be used
 * @param bb_clk_timer  piconet clk[27:0] (unit : half slot)
 * @param bb_clk_us     count up timer (range 0~624 us) \n
 * \p if bb_clk_timer[0] = 1, 312 <= bb_clk_us <= 624; \n
 * \p if bb_clk_timer[0] = 0, 0 <= bb_clk_us <= 312;
 */
extern uint8_t (*rws_read_bt_piconet_clk)(uint16_t conn_handle, uint32_t *bb_clk_timer,
                                          uint16_t *bb_clk_us);


extern bool (*bt_clk_index_read)(uint16_t conn_handle, uint8_t *clk_index, uint8_t *role);

/**
 * @brief Get native clock
 * \xrefitem Added_API_2_14_1_0 "Added Since 2.14.1.0" "Added API"
 *
 * @param bb_clock   clock tick
 * @param bb_clk_us  count up timer
 */
extern void (*lc_get_high_dpi_native_clock)(uint32_t *, uint16_t *);


bool get_airplane_mode(void);
void set_airplane_mode(bool airplane_mode);

#if defined (__cplusplus)
}
#endif

/** End of HAL_87x3E_BT_MAC
  * @}
  */

#endif /* ! BT_MAC_H_ */
