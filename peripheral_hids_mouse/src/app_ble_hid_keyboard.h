#ifndef __APP_BLE_HID_KEYBOARD__
#define __APP_BLE_HID_KEYBOARD__

#include "app_ble_hid_cfg.h"

void hids_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn, bool write);
int hid_buttons_release(const uint8_t *keys, size_t cnt);
int hid_buttons_press(const uint8_t *keys, size_t cnt);
void hids_boot_kb_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn, bool write);

#endif  /*__APP_BLE_HID_KEYBOARD__*/