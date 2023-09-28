#ifndef __APP_BLE_HID_MOUSE__
#define __APP_BLE_HID_MOUSE__

#include "app_ble_hid_cfg.h"

struct mouse_pos {
	int16_t x_val;
	int16_t y_val;
    uint8_t check;
};



extern struct  k_msgq hids_queue;

void mouse_handler(struct k_work *work);
#endif  /*__APP_BLE_HID_MOUSE__*/