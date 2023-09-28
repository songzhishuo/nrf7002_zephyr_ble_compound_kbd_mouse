/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "app_ble_hid_cfg.h"
#include "app_ble_hid_mouse.h"
#include "app_ble_hid_keyboard.h"
#include "app_pin_cfg.h"



#if CONFIG_BT_DIRECTED_ADVERTISING
/* Bonded address queue. */
K_MSGQ_DEFINE(bonds_queue,
			  sizeof(bt_addr_le_t),
			  CONFIG_BT_MAX_PAIRED,
			  4);
#endif

/*广播数据*/
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
				  (BT_APPEARANCE_GENERIC_HID >> 0) & 0xff,
				  (BT_APPEARANCE_GENERIC_HID >> 8) & 0xff),
	// 	      (BT_APPEARANCE_HID_KEYBOARD >> 0) & 0xff,
	// 	      (BT_APPEARANCE_HID_KEYBOARD >> 8) & 0xff),

	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};



static struct k_work adv_work;

static struct k_work pairing_work;
struct pairing_data_mitm
{
	struct bt_conn *conn;
	unsigned int passkey;
};



K_MSGQ_DEFINE(mitm_queue,
			  sizeof(struct pairing_data_mitm),
			  CONFIG_BT_HIDS_MAX_CLIENT_COUNT,
			  4);



static const uint8_t report_map[] = {
#if 1		
    0x05, 0x01,     /* Usage Page (Generic Desktop) */
    0x09, 0x02,     /* Usage (Mouse) */

    0xA1, 0x01,     /* Collection (Application) */

    /* Report ID 1: Mouse buttons + scroll/pan */
    0x85, 0x01,       /* Report Id 1 */
    0x09, 0x01,       /* Usage (Pointer) */		
    0xA1, 0x00,       /* Collection (Physical) */
    0x05, 0x09,       /* Usage Page (Buttons) */
    0x19, 0x01,       //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,       //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,       /* Logical Minimum (0) */
    0x25, 0x01,       /* Logical Maximum (1) */		
    0x95, 0x03,       /* Report Count (3) */
    0x75, 0x01,       /* Report Size (1) */
    0x81, 0x02,       /* Input (Data, Variable, Absolute) */

    0x95, 0x01,       /* Report Count (1) */
    0x75, 13,         /* Report Size (13) */
    0x81, 0x01,       /* Input (Constant) for padding */
    
    0x95, 0x01,       /* Report Count (1) */
    0x75, 0x08,       /* Report Size (8) */
    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x38,       /* Usage (Wheel) */
    0x15, 0x81,       /* Logical Minimum (-127) */
    0x25, 0x7F,       /* Logical Maximum (127) */
    0x81, 0x06,       /* Input (Data, Variable, Relative) */
    0xC0,             /* End Collection (Physical) */

    /* Report ID 2: Mouse motion */
    0x85, 0x02,       /* Report Id 2 */
    0x09, 0x01,       /* Usage (Pointer) */
    0xA1, 0x00,       /* Collection (Physical) */
    0x75, 0x0C,       /* Report Size (12) */
    0x95, 0x02,       /* Report Count (2) */
    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x30,       /* Usage (X) */
    0x09, 0x31,       /* Usage (Y) */
    0x16, 0x01, 0xF8, /* Logical maximum (2047) */
    0x26, 0xFF, 0x07, /* Logical minimum (-2047) */
    0x81, 0x06,       /* Input (Data, Variable, Relative) */
    0xC0,             /* End Collection (Physical) */
    0xC0,              /* End Collection */

/**keyboard*/
    0x05, 0x01,       /* Usage Page (Generic Desktop) */
    0x09, 0x06,       /* Usage (Keyboard) */
    0xA1, 0x01,       /* Collection (Application) */
    0x85, 0x03,			/* Report Id (3) */

    0x05, 0x07,       /* Usage Page (Key Codes) */
    0x19, 0xe0,       /* Usage Minimum (224) */
    0x29, 0xe7,       /* Usage Maximum (231) */
    0x15, 0x00,       /* Logical Minimum (0) */
    0x25, 0x01,       /* Logical Maximum (1) */
    0x75, 0x01,       /* Report Size (1) */
    0x95, 0x08,       /* Report Count (8) */
    0x81, 0x02,       /* Input (Data, Variable, Absolute) */

    0x95, 0x01,       /* Report Count (1) */
    0x75, 0x08,       /* Report Size (8) */
    0x81, 0x01,       /* Input (Constant) reserved byte(1) */

    0x95, 0x06,       /* Report Count (6) */
    0x75, 0x08,       /* Report Size (8) */
    0x15, 0x00,       /* Logical Minimum (0) */
    0x25, 0x65,       /* Logical Maximum (101) */
    0x05, 0x07,       /* Usage Page (Key codes) */
    0x19, 0x00,       /* Usage Minimum (0) */
    0x29, 0x65,       /* Usage Maximum (101) */
    0x81, 0x00,       /* Input (Data, Array) Key array(6 bytes) */   

    /* LED */
    0x85, 0x03,                     /* Report Id (3) */
    0x95, 0x05,                                          /* Report Count (5) */
    0x75, 0x01,                                          /* Report Size (1) */
    0x05, 0x08,                                          /* Usage Page (Page# for LEDs) */
    0x19, 0x01,                                          /* Usage Minimum (1) */
    0x29, 0x05,                                          /* Usage Maximum (5) */
    0x91, 0x02, /* Output (Data, Variable, Absolute), */ /* Led report */
    0x95, 0x01,                                          /* Report Count (1) */
    0x75, 0x03,                                          /* Report Size (3) */
    0x91, 0x01, /* Output (Data, Variable, Absolute), */ /* Led report padding */
    0xC0,                                                /* End Collection (Application) */
#endif
};

#if CONFIG_BT_DIRECTED_ADVERTISING
static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	int err;

	/* Filter already connected peers. */
	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{
		if (conn_mode[i].conn)
		{
			const bt_addr_le_t *dst =
				bt_conn_get_dst(conn_mode[i].conn);

			if (!bt_addr_le_cmp(&info->addr, dst))
			{
				return;
			}
		}
	}

	err = k_msgq_put(&bonds_queue, (void *)&info->addr, K_NO_WAIT);
	if (err)
	{
		printk("No space in the queue for the bond.\n");
	}
}
#endif

static void advertising_continue(void)
{
	struct bt_le_adv_param adv_param;

#if CONFIG_BT_DIRECTED_ADVERTISING
	bt_addr_le_t addr;

	if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT))
	{
		char addr_buf[BT_ADDR_LE_STR_LEN];

		adv_param = *BT_LE_ADV_CONN_DIR(&addr);
		adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

		int err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);

		if (err)
		{
			printk("Directed advertising failed to start\n");
			return;
		}

		bt_addr_le_to_str(&addr, addr_buf, BT_ADDR_LE_STR_LEN);
		printk("Direct advertising to %s started\n", addr_buf);
	}
	else
#endif
	{
		int err;

		adv_param = *BT_LE_ADV_CONN;
		adv_param.options |= BT_LE_ADV_OPT_ONE_TIME;
		err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
							  sd, ARRAY_SIZE(sd));
		if (err)
		{
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}

		printk("Regular advertising started\n");
	}
}

static void advertising_start(void)
{
#if CONFIG_BT_DIRECTED_ADVERTISING
	k_msgq_purge(&bonds_queue);
	bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);
#endif

	k_work_submit(&adv_work);
}

static void advertising_process(struct k_work *work)
{
	advertising_continue();
}

static void pairing_process(struct k_work *work)
{
	int err;
	struct pairing_data_mitm pairing_data;

	char addr[BT_ADDR_LE_STR_LEN];

	err = k_msgq_peek(&mitm_queue, &pairing_data);
	if (err)
	{
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(pairing_data.conn),
					  addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, pairing_data.passkey);
	printk("Press Button 1 to confirm, Button 2 to reject.\n");
}

static void insert_conn_object(struct bt_conn *conn)
{
	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{
		if (!conn_mode[i].conn)
		{
			conn_mode[i].conn = conn;
			conn_mode[i].in_boot_mode = false;

			return;
		}
	}

	printk("Connection object could not be inserted %p\n", conn);
}

static bool is_conn_slot_free(void)
{
	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{
		if (!conn_mode[i].conn)
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief BLE 设备连接回调
 *
 * @param conn
 * @param err
 */
static void connected_callback(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err)
	{
		if (err == BT_HCI_ERR_ADV_TIMEOUT)
		{
			printk("Direct advertising to %s timed out\n", addr);
			k_work_submit(&adv_work);
		}
		else
		{
			printk("Failed to connect to %s (%u)\n", addr, err);
		}
		return;
	}

	printk("Connected %s\n", addr);

	err = bt_hids_connected(get_hid_obj(), conn);

	if (err)
	{
		printk("Failed to notify HID service about connection\n");
		return;
	}

	insert_conn_object(conn);

	if (is_conn_slot_free())
	{
		advertising_start();
	}
}

/**
 * @brief BLE 设备断链回调
 *
 * @param conn
 * @param reason
 */
static void disconnected_callback(struct bt_conn *conn, uint8_t reason)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason %u)\n", addr, reason);

	err = bt_hids_disconnected(get_hid_obj(), conn);

	if (err)
	{
		printk("Failed to notify HID service about disconnection\n");
	}

	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{
		if (conn_mode[i].conn == conn)
		{
			conn_mode[i].conn = NULL;
			break;
		}
	}

	advertising_start();
}

#ifdef CONFIG_BT_HIDS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level,
							 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err)
	{
		printk("Security changed: %s level %u\n", addr, level);
	}
	else
	{
		printk("Security failed: %s level %u err %d\n", addr, level,
			   err);
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_callback,
	.disconnected = disconnected_callback,
#ifdef CONFIG_BT_HIDS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

/**
 * @brief HID模式回调， 告知当前是normal模式 or boot模式
 *
 * @param evt
 * @param conn
 */
static void hids_pm_evt_handler(enum bt_hids_pm_evt evt,
								struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	size_t i;

	for (i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{
		if (conn_mode[i].conn == conn)
		{
			break;
		}
	}

	if (i >= CONFIG_BT_HIDS_MAX_CLIENT_COUNT)
	{
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	switch (evt)
	{
	case BT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		printk("Boot mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = true;
		break;

	case BT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		printk("Report mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = false;
		break;

	default:
		break;
	}
}

static void hid_init(void)
{
	int err;
	struct bt_hids_init_param hids_init_param = {0};
	struct bt_hids_inp_rep *hids_inp_rep;
	struct bt_hids_outp_feat_rep *hids_outp_rep;
	static const uint8_t mouse_movement_mask[DIV_ROUND_UP(INPUT_REP_MOVEMENT_LEN, 8)] = {0};

	hids_init_param.rep_map.data = report_map;
	hids_init_param.rep_map.size = sizeof(report_map);

	hids_init_param.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_param.info.b_country_code = 0x00;
	hids_init_param.info.flags = (BT_HIDS_REMOTE_WAKE |
								  BT_HIDS_NORMALLY_CONNECTABLE);

	hids_inp_rep = &hids_init_param.inp_rep_group_init.reports[INPUT_REP_KEYS_IDX]; // 0
#if 1
	hids_inp_rep->size = INPUT_REP_BUTTONS_LEN;
	hids_inp_rep->id = INPUT_REP_REF_BUTTONS_ID;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_inp_rep++; // 1
	hids_inp_rep->size = INPUT_REP_MOVEMENT_LEN;
	hids_inp_rep->id = INPUT_REP_REF_MOVEMENT_ID;
	hids_inp_rep->rep_mask = mouse_movement_mask;
	hids_init_param.inp_rep_group_init.cnt++;

	hids_inp_rep++; // 2 keyboard
#endif

	hids_inp_rep->size = INPUT_REPORT_KEYS_MAX_LEN;
	hids_inp_rep->id = 3; // INPUT_REP_KEYS_REF_ID;			//3
	hids_init_param.inp_rep_group_init.cnt++;

#if 1
	hids_outp_rep = &hids_init_param.outp_rep_group_init.reports[OUTPUT_REP_KEYS_IDX];
	hids_outp_rep->size = OUTPUT_REPORT_MAX_LEN;
	hids_outp_rep->id = OUTPUT_REP_KEYS_REF_ID;
	hids_outp_rep->handler = hids_outp_rep_handler;
	hids_init_param.outp_rep_group_init.cnt++;
#endif
	hids_init_param.is_mouse = true;
	hids_init_param.is_kb = true;
	hids_init_param.boot_kb_outp_rep_handler = hids_boot_kb_outp_rep_handler;

	hids_init_param.pm_evt_handler = hids_pm_evt_handler;

	err = bt_hids_init(get_hid_obj(), &hids_init_param);
	__ASSERT(err == 0, "HIDS initialization failed\n");
}

#if defined(CONFIG_BT_HIDS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	int err;

	struct pairing_data_mitm pairing_data;

	pairing_data.conn = bt_conn_ref(conn);
	pairing_data.passkey = passkey;

	err = k_msgq_put(&mitm_queue, &pairing_data, K_NO_WAIT);
	if (err)
	{
		printk("Pairing queue is full. Purge previous data.\n");
	}

	/* In the case of multiple pairing requests, trigger
	 * pairing confirmation which needed user interaction only
	 * once to avoid display information about all devices at
	 * the same time. Passkey confirmation for next devices will
	 * be proccess from queue after handling the earlier ones.
	 */
	if (k_msgq_num_used_get(&mitm_queue) == 1)
	{
		k_work_submit(&pairing_work);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct pairing_data_mitm pairing_data;

	if (k_msgq_peek(&mitm_queue, &pairing_data) != 0)
	{
		return;
	}

	if (pairing_data.conn == conn)
	{
		bt_conn_unref(pairing_data.conn);
		k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT);
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif /* defined(CONFIG_BT_HIDS_SECURITY_ENABLED) */

/**
 * @brief
 *
 * @param accept
 */
static void num_comp_reply(bool accept)
{
	struct pairing_data_mitm pairing_data;
	struct bt_conn *conn;

	if (k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT) != 0)
	{
		return;
	}

	conn = pairing_data.conn;

	if (accept)
	{
		bt_conn_auth_passkey_confirm(conn);
		printk("Numeric Match, conn %p\n", conn);
	}
	else
	{
		bt_conn_auth_cancel(conn);
		printk("Numeric Reject, conn %p\n", conn);
	}

	bt_conn_unref(pairing_data.conn);

	if (k_msgq_num_used_get(&mitm_queue))
	{
		k_work_submit(&pairing_work);
	}
}

/**
 * @brief 按键状态处理
 *
 * @param button_state
 * @param has_changed
 */
void button_changed(uint32_t button_state, uint32_t has_changed)
{
	bool data_to_send = false;
	struct mouse_pos pos;
	uint32_t buttons = button_state & has_changed;

	memset(&pos, 0, sizeof(struct mouse_pos));
	if (cfg_flag == 1) /*已经进行了连接配置*/
	{
		return;
	}

	/*开机没有进行配置*/
	if (IS_ENABLED(CONFIG_BT_HIDS_SECURITY_ENABLED))
	{
		if (k_msgq_num_used_get(&mitm_queue))
		{
			cfg_flag = 1;
			if (buttons & KEY_PAIRING_ACCEPT)
			{
				num_comp_reply(true);

				return;
			}

			if (buttons & KEY_PAIRING_REJECT)
			{
				num_comp_reply(false);

				return;
			}
		}
	}
#if 0
	if (buttons & KEY_LEFT_MASK) {
		pos.x_val -= MOVEMENT_SPEED;
		printk("%s(): left\n", __func__);
		data_to_send = true;
	}
	if (buttons & KEY_UP_MASK) {
		pos.y_val -= MOVEMENT_SPEED;
		printk("%s(): up\n", __func__);
		data_to_send = true;
	}
	if (buttons & KEY_RIGHT_MASK) {
		pos.x_val += MOVEMENT_SPEED;
		printk("%s(): right\n", __func__);
		data_to_send = true;
	}
	if (buttons & KEY_DOWN_MASK) {
		pos.y_val += MOVEMENT_SPEED;
		printk("%s(): down\n", __func__);
		data_to_send = true;
	}

	if (data_to_send) {
		int err;

		err = k_msgq_put(&hids_queue, &pos, K_NO_WAIT);
		if (err) {
			printk("No space in the queue for button pressed\n");
			return;
		}
		if (k_msgq_num_used_get(&hids_queue) == 1) {
			k_work_submit(&hids_work);
		}
	}
#endif
}

void configure_buttons(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err)
	{
		printk("Cannot init buttons (err: %d)\n", err);
	}
}

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level)
	{
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}


int main(void)
{
	int err;

	printk("Starting Bluetooth Peripheral HIDS mouse example\n");

	if (IS_ENABLED(CONFIG_BT_HIDS_SECURITY_ENABLED))
	{
		err = bt_conn_auth_cb_register(&conn_auth_callbacks);
		if (err)
		{
			printk("Failed to register authorization callbacks.\n");
			return 0;
		}

		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err)
		{
			printk("Failed to register authorization info callbacks.\n");
			return 0;
		}
	}

	/* DIS initialized at system boot with SYS_INIT macro. */
	hid_init();

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	k_work_init(&hids_work, mouse_handler);
	k_work_init(&adv_work, advertising_process);
	if (IS_ENABLED(CONFIG_BT_HIDS_SECURITY_ENABLED))
	{
		k_work_init(&pairing_work, pairing_process);
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	advertising_start();

	configure_buttons();

	while (1)
	{
		k_sleep(K_MSEC(10));

		app_button_changed(1, 1);

		/* Battery level simulation */
		/// bas_notify();
	}
}
