#include "app_ble_hid_keyboard.h"
static struct keyboard_state {
	uint8_t ctrl_keys_state; 				/*按键当前状态*/
	uint8_t keys_state[KEY_PRESS_MAX];		/*按键键值*/
} hid_keyboard_state;



static int key_report_send(void);
static int hid_kbd_state_key_set(uint8_t key);
static int hid_kbd_state_key_clear(uint8_t key);
static void caps_lock_handler(const struct bt_hids_rep *rep);
/**
 * @brief LED 状态回调 主机-》DEV
 * 
 * @param rep 
 * @param conn 
 * @param write 
 */
void hids_boot_kb_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn, bool write)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!write)
	{
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Boot Keyboard Output report has been received %s\n", addr);
	caps_lock_handler(rep);
}

/**
 * @brief 按键按下指令发送函数
 *
 * @param keys 按键键值
 * @param cnt 键值数量
 * @return int
 */
int hid_buttons_press(const uint8_t *keys, size_t cnt)
{
	while (cnt--)
	{
		int err;

		err = hid_kbd_state_key_set(*keys++);
		if (err)
		{
			printk("Cannot set selected key.\n");
			return err;
		}
	}

	return key_report_send();
}

/**
 * @brief 按键释放指令发送函数
 *
 * @param keys
 * @param cnt
 * @return int
 */
int hid_buttons_release(const uint8_t *keys, size_t cnt)
{
	while (cnt--)
	{
		int err;

		err = hid_kbd_state_key_clear(*keys++);
		if (err)
		{
			printk("Cannot clear selected key.\n");
			return err;
		}
	}

	return key_report_send();
}

/**
 * @brief 主机发出的LED通知
 *
 * @param rep
 * @param conn
 * @param write
 */
void hids_outp_rep_handler(struct bt_hids_rep *rep, struct bt_conn *conn, bool write)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!write)
	{
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Output report has been received %s\n", addr);
	caps_lock_handler(rep);
}

static void caps_lock_handler(const struct bt_hids_rep *rep)
{
	uint8_t report_val = ((*rep->data) & OUTPUT_REPORT_BIT_MASK_CAPS_LOCK) ? 1 : 0;
	dk_set_led(LED_CAPS_LOCK, report_val);
}


static int key_report_con_send(const struct keyboard_state *state,
							   bool boot_mode,
							   struct bt_conn *conn)
{
	int err = 0;
	uint8_t data[INPUT_REPORT_KEYS_MAX_LEN];
	uint8_t *key_data;
	const uint8_t *key_state;
	size_t n;

	data[0] = state->ctrl_keys_state;
	data[1] = 0;
	key_data = &data[2];
	key_state = state->keys_state;

	for (n = 0; n < KEY_PRESS_MAX; ++n)
	{
		*key_data++ = *key_state++;
	}
	if (boot_mode)
	{
		err = bt_hids_boot_kb_inp_rep_send(get_hid_obj(), conn, data,
										   sizeof(data), NULL);
	}
	else
	{
		err = bt_hids_inp_rep_send(get_hid_obj(), conn,
								   2, data, // 这里的0 对应report_map 中的 Report Id INPUT_REP_KEYS_IDX
								   sizeof(data), NULL);
	}
	return err;
}

static uint8_t button_ctrl_code(uint8_t key)
{
	if (KEY_CTRL_CODE_MIN <= key && key <= KEY_CTRL_CODE_MAX)
	{
		return (uint8_t)(1U << (key - KEY_CTRL_CODE_MIN));
	}
	return 0;
}

/**
 * @brief HID keyboard 发送数据
 *
 * @param key
 * @return int
 */
static int hid_kbd_state_key_set(uint8_t key)
{
	uint8_t ctrl_mask = button_ctrl_code(key);

	if (ctrl_mask)
	{
		hid_keyboard_state.ctrl_keys_state |= ctrl_mask;
		return 0;
	}
	for (size_t i = 0; i < KEY_PRESS_MAX; ++i)
	{
		if (hid_keyboard_state.keys_state[i] == 0)
		{
			hid_keyboard_state.keys_state[i] = key;
			return 0;
		}
	}
	/* All slots busy */
	return -EBUSY;
}

/**
 * @brief HID keyboard 发送数据清空
 *
 * @param key 键值
 * @return int
 */
static int hid_kbd_state_key_clear(uint8_t key)
{
	uint8_t ctrl_mask = button_ctrl_code(key);

	if (ctrl_mask)
	{
		hid_keyboard_state.ctrl_keys_state &= ~ctrl_mask;
		return 0;
	}
	for (size_t i = 0; i < KEY_PRESS_MAX; ++i)
	{
		if (hid_keyboard_state.keys_state[i] == key)
		{
			hid_keyboard_state.keys_state[i] = 0;
			return 0;
		}
	}
	/* Key not found */
	return -EINVAL;
}

/**
 * @brief 按键发送函数
 *
 * @return int
 */
static int key_report_send(void)
{
	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{
		if (conn_mode[i].conn)
		{
			int err;

			err = key_report_con_send(&hid_keyboard_state,
									  conn_mode[i].in_boot_mode,
									  conn_mode[i].conn);
			if (err)
			{
				printk("Key report send error: %d\n", err);
				return err;
			}
		}
	}
	return 0;
}
