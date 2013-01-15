/*
 * drivers/input/touchscreen/tm1726.c
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *	Zhenwu Xue <zwxue@ambarella.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c/tm1726.h>

#ifdef	CONFIG_DEBUG_TOUCHSCREEN
#define TM1726_DEBUG(format, arg...)	printk(format , ## arg)
#else
#define TM1726_DEBUG(format, arg...)
#endif

#define	MAX_Z		16
#define MAX_FINGERS	5

typedef enum {
	TM_FINGER_STATE1	= 0x01,
	TM_FINGER_STATE2,
	TM_X1_HIGH,
	TM_Y1_HIGH,
	TM_XY1_LOW,
	TM_W1,
	TM_Z1,
	TM_X2_HIGH,
	TM_Y2_HIGH,
	TM_XY2_LOW,
	TM_W2,
	TM_Z2,
	TM_X3_HIGH,
	TM_Y3_HIGH,
	TM_XY3_LOW,
	TM_W3,
	TM_Z3,
	TM_X4_HIGH,
	TM_Y4_HIGH,
	TM_XY4_LOW,
	TM_W4,
	TM_Z4,
	TM_X5_HIGH,
	TM_Y5_HIGH,
	TM_XY5_LOW,
	TM_W5,
	TM_Z5,

	TM_IRQ_STATUS_ABS	= 0x14,
	TM_DEV_CNTL		= 0x35,
	TM_IRQ_ENABLE_ABS	= 0x36,
	TM_SENSITIVITY_ADJUST	= 0x82,
	TM_RESET		= 0x88,
	TM_FAMILY_CODE		= 0x95,
} tm1726_sub_addr_t;

#define NUM_DATA			32

struct tm1726 {
	char				phys[32];
	struct input_dev		*input;
	struct i2c_client		*client;
	struct workqueue_struct 	*workqueue;
	struct work_struct		report_worker;
	u8				reg_data[NUM_DATA];
	int				irq;
	struct tm1726_fix_data		fix;
	int				(*get_pendown_state)(void);
	void				(*clear_penirq)(void);
};

static int tm1726_reset(struct tm1726 *tm)
{
	u8			buf[1] = {0x00};

	if (i2c_smbus_write_i2c_block_data(tm->client,
		TM_RESET, 1, buf)) {
		printk("I2C Error: %s\n", __func__);
		return -EIO;
	}

	buf[0] = 0x80;
	if (i2c_smbus_write_i2c_block_data(tm->client,
		TM_DEV_CNTL, 1, buf)) {
		printk("I2C Error: %s\n", __func__);
		return -EIO;
	} else {
		return 0;
	}
}

static int tm1726_read_family_code(struct tm1726 *tm, u8 *family_code)
{
	if (i2c_smbus_read_i2c_block_data(tm->client,
		TM_FAMILY_CODE, 1, family_code) != 1) {
		printk("I2C Error: %s\n", __func__);
		return -EIO;
	} else {
		return 0;
	}
}

static int tm1726_config_irq(struct tm1726 *tm)
{
	u8			buf[5] = {0x04, 0x00, 0x0b, 0x04, 0x04};

	if (i2c_smbus_write_i2c_block_data(tm->client,
		TM_IRQ_ENABLE_ABS, 5, buf)) {
		printk("I2C Error: %s\n", __func__);
		return -EIO;
	} else {
		return 0;
	}
}

static int tm1726_adjust_sensitivity(struct tm1726 *tm)
{
	u8			buf[1] = {0x0c};

	if (i2c_smbus_write_i2c_block_data(tm->client,
		TM_SENSITIVITY_ADJUST, 1, buf)) {
		printk("I2C Error: %s\n", __func__);
		return -EIO;
	} else {
		return 0;
	}
}

static inline int tm1726_read_all(struct tm1726 *tm)
{
	if (i2c_smbus_read_i2c_block_data(tm->client,
		TM_IRQ_STATUS_ABS, NUM_DATA, tm->reg_data) != NUM_DATA) {
		printk("I2C Error: %s\n", __func__);
		return -EIO;
	} else {
		return 0;
	}
}

static void tm1726_send_event(struct tm1726 *tm)
{
	struct input_dev	*input = tm->input;
	u8			i, fingers, finger_state[MAX_FINGERS];
	static int		prev_touch = 0;
	static int		curr_touch = 0;
	int			event = 0;

	finger_state[0] = tm->reg_data[TM_FINGER_STATE1] & 0x03;
	finger_state[1] = (tm->reg_data[TM_FINGER_STATE1] & 0x0c) >> 2;
	finger_state[2] = (tm->reg_data[TM_FINGER_STATE1] & 0x30) >> 4;
	finger_state[3] = (tm->reg_data[TM_FINGER_STATE1] & 0xc0) >> 6;
	finger_state[4] = tm->reg_data[TM_FINGER_STATE2] & 0x03;

	curr_touch = 0;
	for (i = 0; i < MAX_FINGERS; i++) {
		if (finger_state[i] == 1 || finger_state[i] == 2) {
			curr_touch++;
		}
	}

	/* Button Pressed */
	if (!prev_touch && curr_touch) {
		input_report_abs(input, BTN_TOUCH, curr_touch);
		TM1726_DEBUG("Finger Pressed\n");
	}

	/* Button Released */
	if (prev_touch && !curr_touch) {
		event = 1;
		input_report_abs(input, ABS_PRESSURE, 0);
		input_report_abs(input, BTN_TOUCH, 0);
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(input);
		TM1726_DEBUG("Finger Released\n\n\n");
	}

	fingers = 0;
	for (i = 0; i < MAX_FINGERS; i++) {
		if (finger_state[i] == 1 || finger_state[i] == 2) {
			u8	xh, xl, yh, yl;
			u32	x, y;

			fingers++;

			xh	= tm->reg_data[TM_X1_HIGH + 5 * i];
			xl	= tm->reg_data[TM_XY1_LOW + 5 * i] & 0x0f;
			yh	= tm->reg_data[TM_Y1_HIGH + 5 * i];
			yl	= (tm->reg_data[TM_XY1_LOW + 5 * i] & 0xf0) >> 4;
			x	= (xh << 4) | xl;
			y	= (yh << 4) | yl;
			TM1726_DEBUG("Finger%d Raw: (%d, %d)\n", fingers, x, y);

			if (x < tm->fix.x_min) {
				x = tm->fix.x_min;
			}
			if (x > tm->fix.x_max) {
				x = tm->fix.x_max;
			}
			if (y < tm->fix.y_min) {
				y = tm->fix.y_min;
			}
			if (y > tm->fix.y_max) {
				y = tm->fix.y_max;
			}

			if (tm->fix.x_invert) {
				x = tm->fix.x_max - x + tm->fix.x_min;
			}

			if (tm->fix.y_invert) {
				y = tm->fix.y_max - y + tm->fix.y_min;
			}

			event	= 1;
			if (fingers == 1) {
				input_report_abs(input, ABS_PRESSURE, MAX_Z);
				input_report_abs(input, ABS_X, x);
				input_report_abs(input, ABS_Y, y);
			}

			input_report_abs(input, ABS_MT_TOUCH_MAJOR, MAX_Z);
			input_report_abs(input, ABS_MT_POSITION_X, x);
			input_report_abs(input, ABS_MT_POSITION_Y, y);
			input_mt_sync(input);
			TM1726_DEBUG("Finger%d Calibrated: (%d, %d)\n", fingers, x, y);
		}
	}

	if (event)
		input_sync(input);
	prev_touch = curr_touch;
}

static irqreturn_t tm1726_irq(int irq, void *handle)
{
	struct tm1726 *tm = handle;

	if (tm->get_pendown_state && !tm->get_pendown_state())
		goto tm1726_irq_exit;

	if (tm->clear_penirq) {
		tm->clear_penirq();
	}

	queue_work(tm->workqueue, &tm->report_worker);

tm1726_irq_exit:
	return IRQ_HANDLED;
}

static void tm1726_report_worker(struct work_struct *work)
{
	struct tm1726	*tm;

	tm = container_of(work, struct tm1726, report_worker);

	tm1726_read_all(tm);
	tm1726_send_event(tm);
}

static int tm1726_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct input_dev 		*input_dev;
	struct tm1726 			*tm;
	struct tm1726_platform_data	*pdata;
	int				err;
	u8				family_code;
	tm1726_product_family_t		tm1726_family;

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}
	pdata->init_platform_hw();

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_WRITE_BYTE_DATA | I2C_FUNC_SMBUS_WRITE_BYTE |
		I2C_FUNC_SMBUS_READ_BYTE))
		return -EIO;

	tm = kzalloc(sizeof(struct tm1726), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!tm || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	tm->client = client;
	i2c_set_clientdata(client, tm);
	tm->input = input_dev;
	tm->get_pendown_state = pdata->get_pendown_state;
	tm->clear_penirq = pdata->clear_penirq;
	snprintf(tm->phys, sizeof(tm->phys),
		 "%s/input0", dev_name(&client->dev));

	err = tm1726_reset(tm);
	if (err)
		goto err_free_mem;

	err = tm1726_read_family_code(tm, &family_code);
	if (err)
		goto err_free_mem;

	tm->fix	= pdata->fix[TM1726_FAMILY_0];
	for (tm1726_family = TM1726_FAMILY_0; tm1726_family < TM1726_FAMILY_END; tm1726_family++) {
		if (pdata->fix[tm1726_family].family_code == family_code) {
			tm->fix	= pdata->fix[tm1726_family];
			break;
		}
	}

	err = tm1726_adjust_sensitivity(tm);
	if (err)
		goto err_free_mem;

	err = tm1726_config_irq(tm);
	if (err)
		goto err_free_mem;

	err = tm1726_read_all(tm);
	if (err)
		goto err_free_mem;

	input_dev->name = "Synaptics TM1726 Touchscreen";
	input_dev->phys = tm->phys;
	input_dev->id.bustype = BUS_I2C;
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(EV_ABS, input_dev->evbit);

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_X, tm->fix.x_min, tm->fix.x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, tm->fix.y_min, tm->fix.y_max, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, tm->fix.x_min, tm->fix.x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, tm->fix.y_min, tm->fix.y_max, 0, 0);

	tm->workqueue = create_singlethread_workqueue("tm1726");
	INIT_WORK(&tm->report_worker, tm1726_report_worker);

	tm->irq = client->irq;
	err = request_irq(tm->irq, tm1726_irq, IRQF_TRIGGER_FALLING,
			client->dev.driver->name, tm);
	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", tm->irq);
		goto err_free_mem;
	}

	err = input_register_device(input_dev);
	if (err)
		goto err_free_irq;

	dev_info(&client->dev, "Family code: 0x%02x, Family: %d", family_code, tm1726_family);

	return 0;

 err_free_irq:
	free_irq(tm->irq, tm);
 err_free_mem:
	input_free_device(input_dev);
	kfree(tm);
	return err;
}

static int tm1726_remove(struct i2c_client *client)
{
	struct tm1726			*tm = i2c_get_clientdata(client);
	struct tm1726_platform_data	*pdata = client->dev.platform_data;

	pdata->exit_platform_hw();
	destroy_workqueue(tm->workqueue);
	free_irq(tm->irq, tm);
	input_unregister_device(tm->input);
	kfree(tm);

	return 0;
}

static struct i2c_device_id tm1726_idtable[] = {
	{ "tm1726", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, tm1726_idtable);

static struct i2c_driver tm1726_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "tm1726"
	},
	.id_table	= tm1726_idtable,
	.probe		= tm1726_probe,
	.remove		= tm1726_remove,
};

static int __init tm1726_init(void)
{
	return i2c_add_driver(&tm1726_driver);
}

static void __exit tm1726_exit(void)
{
	i2c_del_driver(&tm1726_driver);
}

module_init(tm1726_init);
module_exit(tm1726_exit);

MODULE_AUTHOR("Zhenwu Xue <zwxue@ambarella.com>");
MODULE_DESCRIPTION("Synaptics TM1726 TouchScreen Driver");
MODULE_LICENSE("GPL");
