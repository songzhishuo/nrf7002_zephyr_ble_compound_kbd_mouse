#include "app_pin_cfg.h"
#include "app_ble_hid_mouse.h"
struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
struct k_work hids_work;
uint8_t cfg_flag = 0;
void app_button_init()
{
	int err;
	if(!gpio_is_ready_dt(&button0))
	{
		printk("button0 error\r\n");
	}

	if(!gpio_is_ready_dt(&button1))
	{
		printk("button1 error\r\n");
	}
	err = gpio_pin_configure_dt(&button0, GPIO_INPUT);
	err = gpio_pin_configure_dt(&button1, GPIO_INPUT);

}

static const uint8_t hello_world_str[] = {
	GET_KEYPAD_CHAR_USAGE('e'),
	GET_KEYPAD_CHAR_USAGE('e'),
	GET_KEYPAD_CHAR_USAGE('t'),
	GET_KEYPAD_CHAR_USAGE('r'),
	GET_KEYPAD_CHAR_USAGE('e'),
	GET_KEYPAD_CHAR_USAGE('e'),
	0x28,
};
  

// static const uint8_t hello_world_str[] = {
// 	0x0b,	/* Key h */
// 	0x08,	/* Key e */
// 	0x0f,	/* Key l */
// 	0x0f,	/* Key l */
// 	0x12,	/* Key o */
// 	0x28,	/* Key Return */
// };

void app_button_changed(uint32_t button_state, uint32_t has_changed)
{
	static uint8_t *chr = hello_world_str;

	static uint8_t i = 0;
	bool data_to_send = false;
	struct mouse_pos pos;

	
	uint32_t buttons = button_state & has_changed;

	memset(&pos, 0, sizeof(struct mouse_pos));

	uint32_t app_button_state = 0;
	uint32_t app_has_changed = 0;

	if(cfg_flag == 0)			/*未进行配置*/
	{
		return ;
	}

	uint32_t val;
	static uint32_t last_val0 = 0;
	val =  gpio_pin_get_dt(&button0);
	if(last_val0 != val)
	{
		last_val0 = val;
		if(val == 1)
		{
			pos.check = 1;
			pos.x_val -= 10;//MOVEMENT_SPEED+10);
			printk("%s(): left\n", __func__);
			printk("button0 check\r\n");
		}
		else
		{
			pos.check = 0;
		}
		data_to_send = true;

	}

	static uint32_t last_val1 = 0;
	val =  gpio_pin_get_dt(&button1);

	if(last_val1 != val)
	{
		last_val1 = val;
		if(val == 1)
		{
			printk("button1 check [%d]\r\n", i);
			hid_buttons_press(&hello_world_str[i], 1);
		}
		else
		{
			printk("button1 release[%d]\r\n", i);
			hid_buttons_release(&hello_world_str[i], 1);

			i++;
			if(i > sizeof(hello_world_str) - 1)
			{
				i = 0;
			}
		}
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
}


