#include "app_io_msg.h"
/////////////////////////////////////////
//#include "app_in_out_box.h"
__weak inline void app_in_out_box_init(void) {}
//__weak inline void app_in_out_box_handle(T_CASE_LOCATION_STATUS local) {}
__weak inline void app_in_out_box_stop_protect(void) {}
////////////////////////////////
//#include "app_nfc.h"
//T_NFC_DATA nfc_data;
__weak inline void app_nfc_init(void) {}
__weak inline void nfc_gpio_intr_handler(void) {}
__weak inline void app_gpio_nfc_handle_msg(T_IO_MSG *io_driver_msg_recv) {}
__weak inline void app_nfc_multi_link_switch_trigger(void) {}
__weak inline void app_gpio_nfc_board_init(void) {}
////////////////////////////
#include "app_sensor.h"
T_SENSOR_LD_DATA sensor_ld_data;
uint8_t (*app_ld_sensor_hook)(void);
__weak inline void sensor_ld_start(void) {}
__weak inline void sensor_ld_stop(void) {}
__weak inline void gsensor_vendor_sl_disable(void) {}
__weak inline void gsensor_vendor_sl_enable(void) {}
__weak inline void app_gsensor_init(void) {}
__weak inline void app_int_psensor_init(void) {}
__weak inline void app_io_sensor_ld_init(void) {}
__weak inline void app_sensor_ld_init(void) {}
__weak inline void app_sensor_reg_timer(void) {}
__weak inline bool app_gsensor_handle_msg(T_IO_MSG *io_driver_msg_recv) {return true;}
__weak inline void app_sensor_ld_handle_msg(T_IO_MSG *io_driver_msg_recv) {}
__weak inline void app_sensor_ld_io_handle_msg(T_IO_MSG *io_driver_msg_recv) {}
__weak inline void gsensor_int_gpio_intr_handler(void) {}
__weak inline void sensor_ld_io_int_gpio_intr_handler(void) {}
__weak inline void app_switch_detect_init(void) {}
__weak inline void gpio_aux_switch_intr_handler(void) {}
__weak inline void gpio_aux_detect_intr_handler(void) {}
__weak inline void app_switch_detect_handle_msg(T_IO_MSG *io_driver_msg_recv) {}
//__weak inline uint8_t app_aux_switch_state_get(void) { return 2; }
//__weak inline uint8_t app_aux_detect_state_get(void) { return 0; }
///////////////
__weak inline bool app_cfu_test_change_sys_cfg(uint16_t id, uint8_t *data, uint8_t length) {return 0;}
