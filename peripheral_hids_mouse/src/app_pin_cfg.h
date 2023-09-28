#ifndef __APP_PIN_CFG_H__
#define __APP_PIN_CFG_H__
#include "app_ble_hid_cfg.h"


extern struct gpio_dt_spec button0;
extern struct gpio_dt_spec button1;
extern struct k_work hids_work;


extern uint8_t cfg_flag;
void app_button_changed(uint32_t button_state, uint32_t has_changed);


#endif /* __APP_PIN_CFG_H__ */