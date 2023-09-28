#include "app_ble_hid_cfg.h"

/* HIDS instance. */
BT_HIDS_DEF(hids_obj,
	    INPUT_REP_BUTTONS_LEN,
	    INPUT_REP_MOVEMENT_LEN,
	    INPUT_REP_MEDIA_PLAYER_LEN);

struct bt_hids* get_hid_obj()
{
    struct bt_hids * p = &hids_obj;
    return p;
}

conn_mode_TypeDef conn_mode[CONFIG_BT_HIDS_MAX_CLIENT_COUNT];