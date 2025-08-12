#ifndef _APP_DONGLE_CUSTOMIZED_VP_H_
#define _APP_DONGLE_CUSTOMIZED_VP_H_

#define BKP2_FLASH_ADDR             0x238A000

void app_dongle_customized_vp_init(void);
void app_dongle_receive_customized_vp_data(uint16_t data_len, uint8_t *p_data);
void app_dongle_write_vp_finish(void);
void app_dongle_receive_erase_cmd(uint8_t *p_data);
#endif
