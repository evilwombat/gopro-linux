/*
 * drivers/input/touchscreen/nt11001.c
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
#include <linux/i2c/nt11001.h>

#ifdef	CONFIG_DEBUG_TOUCHSCREEN
#define NT11001_DEBUG(format, arg...)	printk(format , ## arg)
#else
#define NT11001_DEBUG(format, arg...)
#endif

#define MAX_X	3930
#define MAX_Y	2384
#define MAX_Z	16

typedef enum {
	NT_GESTURE_CODE = 0x00,
	NT_Y1_HIGH,
	NT_Y1_LOW,
	NT_X1_HIGH,
	NT_X1_LOW,
	NT_Y2_HIGH,
	NT_Y2_LOW,
	NT_X2_HIGH,
	NT_X2_LOW,

	NT_OPERATION_CNTL1 = 0x0c,
	NT_OPERATION_CNTL2,
	NT_CHIP_ID,
	NT_SW_VERSION,
} nt11001_sub_addr_t;

#define NUM_DATA			17

struct nt11001 {
	char				phys[32];
	struct input_dev		*input;
	struct i2c_client		*client;
	struct workqueue_struct 	*workqueue;
	struct work_struct		report_worker;
	u8				reg_data[NUM_DATA];
	int				irq;
	struct nt11001_fix_data		fix;
	int				(*get_pendown_state)(void);
	void				(*clear_penirq)(void);
};

static inline int nt11001_read_all(struct nt11001 *nt)
{
	struct i2c_msg		msg;
	int			errorCode;
	u8			buf[] = {NT_GESTURE_CODE};

	msg.addr	= nt->client->addr;
	msg.flags	= nt->client->flags;
	msg.buf		= buf;
	msg.len		= 1;
	errorCode = i2c_transfer(nt->client->adapter, &msg, 1);
	if (errorCode != 1) {
		printk("NT11001: Can't write reg data.\n");
		return -EIO;
	}

	msg.addr	= nt->client->addr;
	msg.flags	= nt->client->flags | I2C_M_RD;
	msg.buf		= nt->reg_data;
	msg.len		= NUM_DATA;
	errorCode = i2c_transfer(nt->client->adapter, &msg, 1);
	if (errorCode != 1) {
		printk("NT11001: Can't read reg data.\n");
		return -EIO;
	}

	return 0;
}

static void nt11001_send_event(struct nt11001 *nt)
{
	struct input_dev	*input = nt->input;
	u8			finger_state[2];
	static int		prev_touch = 0;
	static int		curr_touch = 0;
	int			event = 0;

	finger_state[0] = (nt->reg_data[NT_GESTURE_CODE] & 0x10) >> 4;
	finger_state[1] = (nt->reg_data[NT_GESTURE_CODE] & 0x20) >> 5;
	curr_touch = finger_state[0] + finger_state[1];

	/* Button Pressed */
	if (!prev_touch && curr_touch) {
		input_report_abs(input, BTN_TOUCH, curr_touch);
		NT11001_DEBUG("Finger Pressed\n");
	}

	/* Button Released */
	if (prev_touch && !curr_touch) {
		event = 1;
		input_report_abs(input, ABS_PRESSURE, 0);
		input_report_abs(input, BTN_TOUCH, 0);
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(input);
		NT11001_DEBUG("Finger Released\n\n\n");
	}

	if (curr_touch) {
		u8 x1h, x1l, y1h, y1l;
		u32 x1, y1;

		x1h = nt->reg_data[NT_X1_HIGH] & 0x01;
		x1l = nt->reg_data[NT_X1_LOW]  & 0xff;
		y1h = nt->reg_data[NT_Y1_HIGH] & 0x01;
		y1l = nt->reg_data[NT_Y1_LOW]  & 0xff;
		x1 = (x1h << 8) | x1l;
		y1 = (y1h << 8) | y1l;
		NT11001_DEBUG("Finger1 Raw: (%d, %d)\n", x1, y1);

		if (x1 < nt->fix.x_min) {
			x1 = nt->fix.x_min;
		}
		if (x1 > nt->fix.x_max) {
			x1 = nt->fix.x_max;
		}
		if (y1 < nt->fix.y_min) {
			y1 = nt->fix.y_min;
		}
		if (y1 > nt->fix.y_max) {
			y1 = nt->fix.y_max;
		}

		if (nt->fix.x_invert)
			x1 = nt->fix.x_max - x1 + nt->fix.x_min;

		if (nt->fix.y_invert)
			y1 = nt->fix.y_max - y1 + nt->fix.y_min;

		event = 1;
		input_report_abs(input, ABS_PRESSURE, MAX_Z);
		input_report_abs(input, ABS_X, x1);
		input_report_abs(input, ABS_Y, y1);

		input_report_abs(input, ABS_MT_TOUCH_MAJOR, MAX_Z);
		input_report_abs(input, ABS_MT_POSITION_X, x1);
		input_report_abs(input, ABS_MT_POSITION_Y, y1);
		input_mt_sync(input);
		NT11001_DEBUG("Finger1 Calibrated: (%d, %d)\n", x1, y1);
	}

	if (curr_touch >= 2) {
		u8 x2h, x2l, y2h, y2l;
		u32 x2, y2;

		x2h = nt->reg_data[NT_X2_HIGH] & 0x01;
		x2l = nt->reg_data[NT_X2_LOW]  & 0xff;
		y2h = nt->reg_data[NT_Y2_HIGH] & 0x01;
		y2l = nt->reg_data[NT_Y2_LOW]  & 0xff;
		x2 = (x2h << 8) | x2l;
		y2 = (y2h << 8) | y2l;
		NT11001_DEBUG("Finger2 Raw: (%d, %d)\n", x2, y2);

		if (x2 < nt->fix.x_min) {
			x2 = nt->fix.x_min;
		}
		if (x2 > nt->fix.x_max) {
			x2 = nt->fix.x_max;
		}
		if (y2 < nt->fix.y_min) {
			y2 = nt->fix.y_min;
		}
		if (y2 > nt->fix.y_max) {
			y2 = nt->fix.y_max;
		}

		if (nt->fix.x_invert)
			x2 = nt->fix.x_max - x2 + nt->fix.x_min;

		if (nt->fix.y_invert)
			y2 = nt->fix.y_max - y2 + nt->fix.y_min;

		event = 1;
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, MAX_Z);
		input_report_abs(input, ABS_MT_POSITION_X, x2);
		input_report_abs(input, ABS_MT_POSITION_Y, y2);
		input_mt_sync(input);
		NT11001_DEBUG("Finger2 Calibrated: (%d, %d)\n", x2, y2);
	}

	if (event)
		input_sync(input);
	prev_touch = curr_touch;
}

static irqreturn_t nt11001_irq(int irq, void *handle)
{
	struct nt11001 *nt = handle;

	if (nt->clear_penirq) {
		nt->clear_penirq();
	}

	queue_work(nt->workqueue, &nt->report_worker);

	return IRQ_HANDLED;
}

static void nt11001_report_worker(struct work_struct *work)
{
	struct nt11001	*nt;

	nt = container_of(work, struct nt11001, report_worker);

	nt11001_read_all(nt);
	nt11001_send_event(nt);
}

static int nt11001_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct input_dev 		*input_dev;
	struct nt11001 			*nt;
	struct nt11001_platform_data	*pdata;
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

	nt = kzalloc(sizeof(struct nt11001), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!nt || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	nt->client = client;
	i2c_set_clientdata(client, nt);
	nt->input = input_dev;
	nt->get_pendown_state = pdata->get_pendown_state;
	nt->clear_penirq = pdata->clear_penirq;
	snprintf(nt->phys, sizeof(nt->phys),
		 "%s/input0", dev_name(&client->dev));

	nt->fix	= pdata->fix;
	err = nt11001_read_all(nt);
	if (err)
		goto err_free_mem;

	input_dev->name = "Novatek NT11001 Touchscreen";
	input_dev->phys = nt->phys;
	input_dev->id.bustype = BUS_I2C;
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(EV_ABS, input_dev->evbit);

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_X, nt->fix.x_min, nt->fix.x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, nt->fix.y_min, nt->fix.y_max, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, nt->fix.x_min, nt->fix.x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, nt->fix.y_min, nt->fix.y_max, 0, 0);

	nt->workqueue = create_singlethread_workqueue("nt11001");
	INIT_WORK(&nt->report_worker, nt11001_report_worker);

	nt->irq = client->irq;
	err = request_irq(nt->irq, nt11001_irq, IRQF_TRIGGER_FALLING,
			client->dev.driver->name, nt);
	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", nt->irq);
		goto err_free_mem;
	}

	err = input_register_device(input_dev);
	if (err)
		goto err_free_irq;

	dev_info(&client->dev, "chip id: %d, software version: %d", nt->reg_data[NT_CHIP_ID], nt->reg_data[NT_SW_VERSION]);

	return 0;

 err_free_irq:
	free_irq(nt->irq, nt);
 err_free_mem:
	input_free_device(input_dev);
	kfree(nt);
	return err;
}

static int nt11001_remove(struct i2c_client *client)
{
	struct nt11001			*nt = i2c_get_clientdata(client);
	struct nt11001_platform_data	*pdata = client->dev.platform_data;

	pdata->exit_platform_hw();
	destroy_workqueue(nt->workqueue);
	free_irq(nt->irq, nt);
	input_unregister_device(nt->input);
	kfree(nt);

	return 0;
}

static struct i2c_device_id nt11001_idtable[] = {
	{ "nt11001", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, nt11001_idtable);

static struct i2c_driver nt11001_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "nt11001"
	},
	.id_table	= nt11001_idtable,
	.probe		= nt11001_probe,
	.remove		= nt11001_remove,
};

static int __init nt11001_init(void)
{
	return i2c_add_driver(&nt11001_driver);
}

static void __exit nt11001_exit(void)
{
	i2c_del_driver(&nt11001_driver);
}

module_init(nt11001_init);
module_exit(nt11001_exit);

MODULE_AUTHOR("Zhenwu Xue <zwxue@ambarella.com>");
MODULE_DESCRIPTION("Novatek NT11001 TouchScreen Driver");
MODULE_LICENSE("GPL");

