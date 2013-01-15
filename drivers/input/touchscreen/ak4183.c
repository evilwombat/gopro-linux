/*
 * drivers/input/touchscreen/ak4183.c
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
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c/ak4183.h>

/* ns delay before the first sample */
#define TS_POLL_DELAY			(10 * 1000)
/* ns delay between samples */
#define TS_POLL_PERIOD			(5 * 1000)


#define AK4183_MEASURE_X		(0xc << 4)
#define AK4183_MEASURE_Y		(0xd << 4)
#define AK4183_MEASURE_Z1		(0xe << 4)
#define AK4183_MEASURE_Z2		(0xf << 4)

#define AK4183_AUTO_POWER_IRQ_EN	(0x0 << 2)
#define AK4183_ADC_ON_IRQ_DIS		(0x1 << 2)

#define AK4183_12BIT			(0x0 << 1)
#define AK4183_8BIT			(0x1 << 1)

#define	VAL_12BIT			(1 << 12)
#define	MAX_12BIT			(VAL_12BIT - 1)

#define ADC_ON_12BIT			(AK4183_12BIT | AK4183_AUTO_POWER_IRQ_EN)

#define READ_X				(ADC_ON_12BIT | AK4183_MEASURE_X)
#define READ_Y				(ADC_ON_12BIT | AK4183_MEASURE_Y)
#define READ_Z1				(ADC_ON_12BIT | AK4183_MEASURE_Z1)
#define READ_Z2				(ADC_ON_12BIT | AK4183_MEASURE_Z2)

struct ts_event {
	u16	x;
	u16	y;
	u16	z1, z2;
};

struct ak4183 {
	struct input_dev	*input;
	char			phys[32];
	struct hrtimer		timer;
	struct ts_event		tc;

	struct workqueue_struct *workqueue;
	struct work_struct	report_worker;

	struct i2c_client	*client;

	spinlock_t		lock;

	u16			model;
	u16			x_plate_ohms;
	struct ak4183_fix_data	fix;

	unsigned		pendown;
	int			irq;

	int			(*get_pendown_state)(void);
	void			(*clear_penirq)(void);
};

static inline int ak4183_xfer(struct ak4183 *tsc, u8 cmd)
{
	s32 data;
	u16 val;

	data = i2c_smbus_read_word_data(tsc->client, cmd);
	if (data < 0) {
		dev_err(&tsc->client->dev, "i2c io error: %d\n", data);
		return data;
	}

	/* The protocol and raw data format from i2c interface:
	 * S Addr Wr [A] Comm [A] S Addr Rd [A] [DataLow] A [DataHigh] NA P
	 * Where DataLow has [D11-D4], DataHigh has [D3-D0 << 4 | Dummy 4bit].
	 */
	val = swab16(data) >> 4;

	dev_dbg(&tsc->client->dev, "data: 0x%x, val: 0x%x\n", data, val);

	return val;
}

static void ak4183_send_event(void *tsc)
{
	struct ak4183	*ts = tsc;
	u32		rt;
	u32		x, y, z1, z2;

	x = ts->tc.x;
	y = ts->tc.y;
	z1 = ts->tc.z1;
	z2 = ts->tc.z2;

	/* range filtering */
	if (x >= MAX_12BIT)
		x = 0;

	if (likely(x && z1)) {
		/* compute touch pressure resistance using equation #1 */
		rt = z2;
		rt -= z1;
		rt *= x;
		rt *= ts->x_plate_ohms;
		rt /= z1;
		rt = (rt + 2047) >> 12;
	} else
		rt = 0;

	/* Sample found inconsistent by debouncing or pressure is beyond
	 * the maximum. Don't report it to user space, repeat at least
	 * once more the measurement
	 */
	if (rt > MAX_12BIT) {
		dev_dbg(&ts->client->dev, "ignored pressure %d\n", rt);

		return;
	}

	/* NOTE: We can't rely on the pressure to determine the pen down
	 * state, even this controller has a pressure sensor.  The pressure
	 * value can fluctuate for quite a while after lifting the pen and
	 * in some cases may not even settle at the expected value.
	 *
	 * The only safe way to check for the pen up condition is in the
	 * timer by reading the pen signal state (it's a GPIO _and_ IRQ).
	 */
	if (rt) {
		struct input_dev *input = ts->input;

		if (!ts->pendown) {
			dev_dbg(&ts->client->dev, "DOWN\n");

			input_report_key(input, BTN_TOUCH, 1);
			ts->pendown = 1;
		}

		if (ts->fix.x_rescale) {
			x = (x > ts->fix.x_min) ? (x - ts->fix.x_min) : 0;
			x *= VAL_12BIT;
			x /= (ts->fix.x_max - ts->fix.x_min);
			if (x >= MAX_12BIT)
				x = MAX_12BIT;
		}

		if (ts->fix.y_rescale) {
			y = (y > ts->fix.y_min) ? (y - ts->fix.y_min) : 0;
			y *= VAL_12BIT;
			y /= (ts->fix.y_max - ts->fix.y_min);
			if (y >= MAX_12BIT)
				y = MAX_12BIT;
		}

		if (ts->fix.x_invert)
			x = (MAX_12BIT - x);

		if (ts->fix.y_invert)
			y = (MAX_12BIT - y);

		input_report_abs(input, ABS_X, x);
		input_report_abs(input, ABS_Y, y);
		input_report_abs(input, ABS_PRESSURE, rt);

		input_sync(input);

		dev_dbg(&ts->client->dev,
			"point(%4d,%4d), pressure (%4u), real(%4d,%4d)\n",
			x, y, rt, ts->tc.x, ts->tc.y);
	}
}

static int ak4183_read_values(struct ak4183 *tsc)
{
	/* y- still on; turn on only y+ (and ADC) */
	tsc->tc.y = ak4183_xfer(tsc, READ_Y);

	/* turn y- off, x+ on, then leave in lowpower */
	tsc->tc.x = ak4183_xfer(tsc, READ_X);

	/* turn y+ off, x- on; we'll use formula #1 */
	tsc->tc.z1 = ak4183_xfer(tsc, READ_Z1);
	tsc->tc.z2 = ak4183_xfer(tsc, READ_Z2);

	return 0;
}

static enum hrtimer_restart ak4183_timer(struct hrtimer *handle)
{
	struct ak4183 *ts = container_of(handle, struct ak4183, timer);

	queue_work(ts->workqueue, &ts->report_worker);

	return HRTIMER_NORESTART;
}

static irqreturn_t ak4183_irq(int irq, void *handle)
{
	struct ak4183 *ts = handle;

	if (likely(ts->get_pendown_state())) {
		disable_irq_nosync(ts->irq);
		hrtimer_start(&ts->timer, ktime_set(0, TS_POLL_DELAY),
			HRTIMER_MODE_REL);
	}

	return IRQ_HANDLED;
}

static void ak4183_report_worker(struct work_struct *work)
{
	struct ak4183 *ts;

	ts = container_of(work, struct ak4183, report_worker);

	if (unlikely(!ts->get_pendown_state() && ts->pendown)) {
		struct input_dev *input = ts->input;

		dev_dbg(&ts->client->dev, "UP\n");

		input_report_key(input, BTN_TOUCH, 0);
		input_report_abs(input, ABS_PRESSURE, 0);
		input_sync(input);

		ts->pendown = 0;

		if (ts->clear_penirq)
			ts->clear_penirq();
		enable_irq(ts->irq);
	} else {
		/* pen is still down, continue with the measurement */
		dev_dbg(&ts->client->dev, "pen is still down\n");

		ak4183_read_values(ts);
		ak4183_send_event(ts);

		hrtimer_start(&ts->timer, ktime_set(0, TS_POLL_PERIOD),
			HRTIMER_MODE_REL);
	}
}

static int ak4183_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ak4183 *ts;
	struct ak4183_platform_data *pdata = pdata = client->dev.platform_data;
	struct input_dev *input_dev;
	int err;

	if (!pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

	ts = kzalloc(sizeof(struct ak4183), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	ts->client = client;
	i2c_set_clientdata(client, ts);

	ts->input = input_dev;

	hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ts->timer.function = ak4183_timer;

	spin_lock_init(&ts->lock);

	ts->model             = pdata->model;
	ts->x_plate_ohms      = pdata->x_plate_ohms;
	ts->fix               = pdata->fix;
	ts->get_pendown_state = pdata->get_pendown_state;
	ts->clear_penirq      = pdata->clear_penirq;

	pdata->init_platform_hw();

	snprintf(ts->phys, sizeof(ts->phys),
		 "%s/input0", dev_name(&client->dev));

	input_dev->name = "AK4183 Touchscreen";
	input_dev->phys = ts->phys;
	input_dev->id.bustype = BUS_I2C;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_12BIT, 0, 0);

	ak4183_read_values(ts);

	ts->workqueue = create_singlethread_workqueue("ak4183");
	INIT_WORK(&ts->report_worker, ak4183_report_worker);
	ts->irq = client->irq;

	err = request_irq(ts->irq, ak4183_irq, 0,
			client->dev.driver->name, ts);
	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", ts->irq);
		goto err_free_mem;
	}

	err = input_register_device(input_dev);
	if (err)
		goto err_free_irq;

	dev_info(&client->dev, "registered with irq (%d)\n", ts->irq);

	return 0;

 err_free_irq:
	free_irq(ts->irq, ts);
	hrtimer_cancel(&ts->timer);
 err_free_mem:
	input_free_device(input_dev);
	kfree(ts);
	return err;
}

static int ak4183_remove(struct i2c_client *client)
{
	struct ak4183	*ts = i2c_get_clientdata(client);
	struct ak4183_platform_data *pdata;

	pdata = client->dev.platform_data;
	pdata->exit_platform_hw();

	destroy_workqueue(ts->workqueue);
	free_irq(ts->irq, ts);
	hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input);
	kfree(ts);

	return 0;
}

static struct i2c_device_id ak4183_idtable[] = {
	{ "ak4183", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ak4183_idtable);

static struct i2c_driver ak4183_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ak4183"
	},
	.id_table	= ak4183_idtable,
	.probe		= ak4183_probe,
	.remove		= ak4183_remove,
};

static int __init ak4183_init(void)
{
	return i2c_add_driver(&ak4183_driver);
}

static void __exit ak4183_exit(void)
{
	i2c_del_driver(&ak4183_driver);
}

module_init(ak4183_init);
module_exit(ak4183_exit);

MODULE_AUTHOR("Zhenwu Xue <zwxue@ambarella.com>");
MODULE_DESCRIPTION("AK4183 TouchScreen Driver");
MODULE_LICENSE("GPL");
