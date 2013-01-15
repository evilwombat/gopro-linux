/*
 * drivers/input/touchscreen/cy8ctmg.c
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/i2c/cy8ctmg.h>

#define X_RES	320
#define Y_RES	480
#define MAX_X	(X_RES - 1)
#define MAX_Y	(Y_RES - 1)

typedef enum {
	CY_MODE = 0,
	CY_REVERSE_MODE,
	CY_X1_HIGH,
	CY_X1_LOW,
	CY_Y1_HIGH,
	CY_Y1_LOW,
	CY_X2_HIGH,
	CY_X2_LOW,
	CY_Y2_HIGH,
	CY_Y2_LOW,
	CY_FINGER_TOUCH,
	CY_GESTURE_CODE,
	CY_GESTURE_PARA_HIGH,
	CY_GESTURE_PARA_LOW = 0x0d,
} cy8ctmg_sub_addr_t;

#define SCAN_PROCEEDING 	0x00
#define SCAN_COMPLETE		0x08
#define SEND_INT_WHEN_TOUCHED	0x00
#define SEND_INT_WHEN_SCANNED	0x04
#define SCAN_NORMAL		0x00
#define SCAN_IDLE		0x01
#define SCAN_SLEEP		0x02
#define SCAN_DISABLED		0x03

#define CY_MODE_SCAN_BASED	(SCAN_PROCEEDING | SEND_INT_WHEN_SCANNED | SCAN_NORMAL)

#define CY_SCAN_DATA()	\
	cy8ctmg_write_byte(ts, CY_MODE, CY_MODE_SCAN_BASED);	\
	cy8ctmg_write_byte(ts, CY_REVERSE_MODE, ~CY_MODE_SCAN_BASED);

struct cy8ctmg {
	char				phys[32];
	struct input_dev		*input;
	struct i2c_client		*client;
	struct workqueue_struct 	*workqueue;
	struct work_struct		report_worker;
	int				irq;
	struct cy8ctmg_fix_data		fix;
	int				(*get_pendown_state)(void);
	void				(*clear_penirq)(void);
};

static inline int cy8ctmg_write_byte(struct cy8ctmg *cy, u8 addr, u8 data)
{
	s32 error;

	error = i2c_smbus_write_byte_data(cy->client, addr, data);
	if (error < 0)
		dev_err(&cy->client->dev, "i2c io error: %d\n", error);

	return error;
}

static inline int cy8ctmg_set_sub_addr(struct cy8ctmg *cy, u8 sub_addr)
{
	s32 error;

	error = i2c_smbus_write_byte(cy->client, sub_addr);
	if (error < 0)
		dev_err(&cy->client->dev, "i2c io error: %d\n", error);

	return error;
}

static inline int cy8ctmg_read_byte(struct cy8ctmg *cy)
{
	s32 data;

	data = i2c_smbus_read_byte(cy->client);
	if (data < 0)
		dev_err(&cy->client->dev, "i2c io error: %d\n", data);

	return data;
}

static void cy8ctmg_send_event(void *cy, int touched)
{
	struct cy8ctmg		*ts = cy;
	struct input_dev	*input = ts->input;
	static int		prev_touch = 0;
	static int		curr_touch = 0;
	int			event = 0;

	curr_touch = touched;

	/* Button Pressed */
	if (!prev_touch && curr_touch) {
		event = 1;
		input_report_key(ts->input, BTN_TOUCH, 1);
	}

	/* Button Released */
	if (prev_touch && !curr_touch) {
		event = 1;
		input_report_key(ts->input, BTN_TOUCH, 0);
	}

	if (curr_touch) {
		u8 x1h, x1l, y1h, y1l;
		u32 x1, y1;

		cy8ctmg_set_sub_addr(ts, CY_X1_HIGH);
		x1h = cy8ctmg_read_byte(ts);
		cy8ctmg_set_sub_addr(ts, CY_X1_LOW);
		x1l = cy8ctmg_read_byte(ts);
		cy8ctmg_set_sub_addr(ts, CY_Y1_HIGH);
		y1h = cy8ctmg_read_byte(ts);
		cy8ctmg_set_sub_addr(ts, CY_Y1_LOW);
		y1l = cy8ctmg_read_byte(ts);
		x1 = (x1h << 8) | x1l;
		y1 = (y1h << 8) | y1l;

		if (ts->fix.x_rescale) {
			x1 = (x1 > ts->fix.x_min) ? (x1 - ts->fix.x_min) : 0;
			x1 *= X_RES;
			x1 /= (ts->fix.x_max - ts->fix.x_min);
			if (x1 >= MAX_X)
				x1 = MAX_X;
		}

		if (ts->fix.y_rescale) {
			y1 = (y1 > ts->fix.y_min) ? (y1 - ts->fix.y_min) : 0;
			y1 *= Y_RES;
			y1 /= (ts->fix.y_max - ts->fix.y_min);
			if (y1 >= MAX_Y)
				y1 = MAX_Y;
		}

		if (ts->fix.x_invert)
			x1 = MAX_X - x1;

		if (ts->fix.y_invert)
			y1 = MAX_Y - y1;

		event = 1;
		input_report_abs(input, ABS_X, x1);
		input_report_abs(input, ABS_Y, y1);
	}

	if (event)
		input_sync(input);
	prev_touch = curr_touch;
}

static irqreturn_t cy8ctmg_irq(int irq, void *handle)
{
	struct cy8ctmg *ts = handle;

	if (ts->clear_penirq)
		ts->clear_penirq();

	queue_work(ts->workqueue, &ts->report_worker);

	return IRQ_HANDLED;
}

static void cy8ctmg_report_worker(struct work_struct *work)
{
	struct cy8ctmg	*ts = container_of(work, struct cy8ctmg, report_worker);
	int		touched;

	cy8ctmg_set_sub_addr(ts, CY_FINGER_TOUCH);
	touched = cy8ctmg_read_byte(ts);
	cy8ctmg_send_event(ts, touched);
	msleep(80);
	CY_SCAN_DATA()
}

static int cy8ctmg_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct input_dev 		*input_dev;
	struct cy8ctmg 			*ts;
	struct cy8ctmg_platform_data	*pdata;
	int				err;

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

	ts = kzalloc(sizeof(struct cy8ctmg), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	ts->client = client;
	i2c_set_clientdata(client, ts);
	ts->input = input_dev;
	ts->fix	= pdata->fix;
	ts->clear_penirq = pdata->clear_penirq;
	snprintf(ts->phys, sizeof(ts->phys),
		 "%s/input0", dev_name(&client->dev));

	input_dev->name = "CY8CTMG Touchscreen";
	input_dev->phys = ts->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 0, MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MAX_Y, 0, 0);

	ts->workqueue = create_singlethread_workqueue("cy8ctmg");
	INIT_WORK(&ts->report_worker, cy8ctmg_report_worker);

	ts->irq = client->irq;
	err = request_irq(ts->irq, cy8ctmg_irq, 0, client->dev.driver->name, ts);
	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", ts->irq);
		goto err_free_mem;
	}

	err = input_register_device(input_dev);
	if (err)
		goto err_free_irq;

	dev_info(&client->dev, "registered with irq (%d)\n", ts->irq);

	CY_SCAN_DATA()

	return 0;

 err_free_irq:
	free_irq(ts->irq, ts);
 err_free_mem:
	input_free_device(input_dev);
	kfree(ts);
	return err;
}

static int cy8ctmg_remove(struct i2c_client *client)
{
	struct cy8ctmg			*ts = i2c_get_clientdata(client);
	struct cy8ctmg_platform_data	*pdata = client->dev.platform_data;

	pdata->exit_platform_hw();
	destroy_workqueue(ts->workqueue);
	free_irq(ts->irq, ts);
	input_unregister_device(ts->input);
	kfree(ts);

	return 0;
}

static struct i2c_device_id cy8ctmg_idtable[] = {
	{ "cy8ctmg", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, cy8ctmg_idtable);

static struct i2c_driver cy8ctmg_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "cy8ctmg"
	},
	.id_table	= cy8ctmg_idtable,
	.probe		= cy8ctmg_probe,
	.remove		= cy8ctmg_remove,
};

static int __init cy8ctmg_init(void)
{
	return i2c_add_driver(&cy8ctmg_driver);
}

static void __exit cy8ctmg_exit(void)
{
	i2c_del_driver(&cy8ctmg_driver);
}

module_init(cy8ctmg_init);
module_exit(cy8ctmg_exit);

MODULE_AUTHOR("Zhenwu Xue <zwxue@ambarella.com>");
MODULE_DESCRIPTION("CY8CTMG TouchScreen Driver");
MODULE_LICENSE("GPL");
