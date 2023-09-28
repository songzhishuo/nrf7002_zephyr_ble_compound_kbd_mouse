#include "app_ble_hid_mouse.h"

/* Mouse movement queue. */
K_MSGQ_DEFINE(hids_queue,
	      sizeof(struct mouse_pos),
	      HIDS_QUEUE_SIZE,
	      4);


static void mouse_movement_send(int16_t x_delta, int16_t y_delta, uint8_t check)
{
	for (size_t i = 0; i < CONFIG_BT_HIDS_MAX_CLIENT_COUNT; i++)
	{

		if (!conn_mode[i].conn)
		{
			continue;
		}

		if (conn_mode[i].in_boot_mode)
		{
			x_delta = MAX(MIN(x_delta, SCHAR_MAX), SCHAR_MIN);
			y_delta = MAX(MIN(y_delta, SCHAR_MAX), SCHAR_MIN);

			bt_hids_boot_mouse_inp_rep_send(get_hid_obj(),
											conn_mode[i].conn,
											NULL,
											(int8_t)x_delta,
											(int8_t)y_delta,
											NULL);
		}
		else
		{
			uint8_t x_buff[2];
			uint8_t y_buff[2];
			uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

			int16_t x = MAX(MIN(x_delta, 0x07ff), -0x07ff);
			int16_t y = MAX(MIN(y_delta, 0x07ff), -0x07ff);

			/* Convert to little-endian. */
			sys_put_le16(x, x_buff);
			sys_put_le16(y, y_buff);

			/* Encode report. */
			BUILD_ASSERT(sizeof(buffer) == 3,
						 "Only 2 axis, 12-bit each, are supported");

			// static uint8_t flag = 1;
			buffer[0] = (check&0x01);
			buffer[1] = 0;
			buffer[2] = 0;

			
			bt_hids_inp_rep_send(get_hid_obj(), conn_mode[i].conn,
								 // INPUT_REP_MOVEMENT_INDEX,
								 INPUT_REP_BUTTONS_INDEX, // 这里的0 对应report_map 中的 Report Id 1
								 buffer, sizeof(buffer), NULL);


			

		}
	}
}

/**
 * @brief 按键发送回调
 * 
 * @param work 
 */
void mouse_handler(struct k_work *work)
{
	struct mouse_pos pos;

	while (!k_msgq_get(&hids_queue, &pos, K_NO_WAIT))
	{
		mouse_movement_send(pos.x_val, pos.y_val, pos.check);
	}
}

