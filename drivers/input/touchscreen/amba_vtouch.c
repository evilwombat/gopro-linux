/*
 * drivers/input/touchscreen/amba_vtouch.c
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *	Keny Huang <skhuang@ambarella.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <linux/aipc/i_touch.h>
#include <linux/aipc/lk_touch.h>

#define DEBUG_AMBA_VTOUCH 0
#if DEBUG_AMBA_VTOUCH
#define dbgmsg printk
#else
#define dbgmsg(...) //printk
#endif

struct amba_vtouch {
	struct input_dev *input_dev;
	char phys[32];
	int irq;
};

static struct amba_vtouch *amba_vtouch_dev = NULL;

int amba_vtouch_press_abs_mt_sync(struct amba_vtouch_data *data)
{
	int MAX_Z = 16;
	int MAX_FINGERS = data->max_finger;
	int cur_touch = 0;
	int tmp = 0, i = 0, done = 0;

	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d,%d\n",__func__, data->finger[0].x, data->finger[0].y);

	for (i = 0; i < MAX_FINGERS; i++) {

		if (data->finger[i].press) {
			if (done == 0) {
				input_report_abs(amba_vtouch_dev->input_dev, ABS_PRESSURE, MAX_Z);
				input_report_abs(amba_vtouch_dev->input_dev, ABS_X, data->finger[i].x);
				input_report_abs(amba_vtouch_dev->input_dev, ABS_Y, data->finger[i].y);
				done = 1;
			}
			input_report_abs(amba_vtouch_dev->input_dev, ABS_MT_TOUCH_MAJOR, MAX_Z);
			input_report_abs(amba_vtouch_dev->input_dev, ABS_MT_POSITION_X,
					data->finger[i].x);
			input_report_abs(amba_vtouch_dev->input_dev, ABS_MT_POSITION_Y,
					data->finger[i].y);
			input_mt_sync(amba_vtouch_dev->input_dev);
		}
	}
	//Inform Linux/Android events arrive
	input_sync(amba_vtouch_dev->input_dev);

	return 0;
}
EXPORT_SYMBOL(amba_vtouch_press_abs_mt_sync);

int amba_vtouch_release_abs_mt_sync(struct amba_vtouch_data *data)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d,%d\n",__func__, data->finger[0].x, data->finger[0].y);

	input_report_abs(amba_vtouch_dev->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	input_mt_sync(amba_vtouch_dev->input_dev);

	input_sync(amba_vtouch_dev->input_dev);

	return 0;
}
EXPORT_SYMBOL(amba_vtouch_release_abs_mt_sync);

#if 0
int amba_vtouch_report_key(unsigned int code, int value)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d, %d\n",__func__, code, value);
	input_report_key(amba_vtouch_dev->input_dev, code, value);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_report_key);

int amba_vtouch_report_rel(unsigned int code, int value)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d, %d\n",__func__, code, value);
	input_report_rel(amba_vtouch_dev->input_dev, code, value);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_report_rel);

int amba_vtouch_report_switch(unsigned int code, int value)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d, %d\n",__func__, code, value);
	input_report_switch(amba_vtouch_dev->input_dev, code, value);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_report_switch);

int amba_vtouch_set_capability(unsigned int type, unsigned int code)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d, %d\n",__func__, type, code);
	input_set_capability(amba_vtouch_dev->input_dev, type, code);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_set_capability);

int amba_vtouch_report_abs(unsigned int code, int value)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d, %d\n",__func__, code, value);
	input_report_abs(amba_vtouch_dev->input_dev, code, value);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_report_abs);

int amba_vtouch_sync(void)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s\n",__func__);
	input_sync(amba_vtouch_dev->input_dev);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_sync);

int amba_vtouch_set_abs_params(unsigned int axis,
			  int min, int max, int fuzz, int flat)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d,%d,%d,%d,%d\n",__func__, axis,min,max,fuzz,flat);
	input_set_abs_params(amba_vtouch_dev->input_dev, axis, min, max, fuzz, flat);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_set_abs_params);

int amba_vtouch_mt_sync(void)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s\n",__func__);
	input_mt_sync(amba_vtouch_dev->input_dev);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_mt_sync);

int amba_vtouch_mt_slot(int slot)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d\n",__func__, slot);
	input_mt_slot(amba_vtouch_dev->input_dev, slot);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_mt_slot);

int amba_vtouch_report_ff_status(unsigned int code, int value)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d, %d\n",__func__, code, value);
	input_report_ff_status(amba_vtouch_dev->input_dev, code, value);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_report_ff_status);

int amba_vtouch_set_events_per_packet(int n_events)
{
	if((amba_vtouch_dev==NULL) || (amba_vtouch_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d\n",__func__, n_events);
	input_set_events_per_packet(amba_vtouch_dev->input_dev, n_events);
	return 0;
}
EXPORT_SYMBOL(amba_vtouch_set_events_per_packet);
#endif


/*
 * The functions for inserting/removing us as a module.
 */

static int __devinit amba_vtouch_probe(struct platform_device *pdev)
{
	struct input_dev	*input_dev;
	int		err = 0;

		
	if(amba_vtouch_dev!=NULL){
		dev_err(&pdev->dev, "Amba_vtouch already exists. Skip operation!\n");
		return 0;
	}

	/* Check if touch device valid via IPC */
	err = ipc_itouch_is_module_valid();
	if(err<0){
		dev_err(&pdev->dev, "Amba_vtouch module does not exist. Skip operation!\n");
		return -EINVAL;
	}
	
	/* Allocate memory for device */
	amba_vtouch_dev = kzalloc(sizeof(struct amba_vtouch), GFP_KERNEL);
	if (!amba_vtouch_dev) {
		dev_err(&pdev->dev, "failed to allocate memory.\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, amba_vtouch_dev);

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "failed to allocate input device.\n");
		err = -EBUSY;
		goto err_free_mem;
	}

/*	amba_vtouch_dev->irq = platform_get_irq(pdev, 0);
	if (amba_vtouch_dev->irq < 0) {
		dev_err(&pdev->dev, "no irq ID is designated.\n");
		err = -ENODEV;
		goto err_free_dev;
	}

	err = request_irq(amba_vtouch_dev->irq, amba_vtouch_interrupt, IRQF_DISABLED,
			pdev->dev.driver->name, amba_vtouch_dev);
	if (err) {
		dev_err(&pdev->dev, "failed to allocate irq.\n");
		goto err_unmap_regs;
	}*/

	amba_vtouch_dev->input_dev = input_dev;

	snprintf(amba_vtouch_dev->phys, sizeof(amba_vtouch_dev->phys),
		 "%s/input0", dev_name(&pdev->dev));

	input_dev->name = "Ambarella virtual touch screen controller";
	input_dev->phys = amba_vtouch_dev->phys;
	input_dev->dev.parent = &pdev->dev;
	
	input_dev->id.bustype = BUS_HOST;
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);


#if 0
	/* get init data via IPC and set */
	err = ipc_itouch_init();
	if(err<0){
		dev_err(&pdev->dev, "Amba_vtouch fail to set init value. Skip operation!\n");
		goto err_fail;
	}
#else
{
	int MAX_X = 4095;
	int MAX_Y = 4095;
	int MAX_Z = 16;
	
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_X, 0, MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MAX_Y, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, MAX_Y, 0, 0);
}
#endif
	/* All went ok, so register to the input system */
	err = input_register_device(input_dev);
	if (err){
		dev_err(&pdev->dev, "failed to input_register_device amba_vtouch.\n");
		goto err_fail;
	}
	dev_err(&pdev->dev, "probe amba_vtouch to [%s] done.\n",input_dev->phys);

	return 0;

err_fail:
//	free_irq(amba_vtouch_dev->irq, amba_vtouch_dev);
//err_free_dev:
	input_free_device(amba_vtouch_dev->input_dev);
err_free_mem:
	kfree(amba_vtouch_dev);
	amba_vtouch_dev=NULL;
	return err;
}

static int __devexit amba_vtouch_remove(struct platform_device *pdev)
{
	struct amba_vtouch *t_dev = dev_get_drvdata(&pdev->dev);

	if(t_dev!=amba_vtouch_dev){
		dev_err(&pdev->dev, "%s: Invalid amba_vtouch_dev. Skip operation!\n",__func__);
		return 0;
	}
	
//	free_irq(amba_vtouch_dev->irq, amba_vtouch_dev);

	input_unregister_device(amba_vtouch_dev->input_dev);

	kfree(amba_vtouch_dev);
	amba_vtouch_dev=NULL;

	return 0;
}

struct platform_device amba_vtouch = {
	.name			= "amba_vtouch",
	.id			= -1,
	.resource		= NULL,
	.num_resources		= 0,
	.dev			= {
		.platform_data		= NULL,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
EXPORT_SYMBOL(amba_vtouch);

static struct platform_driver amba_vtouch_driver = {
	.probe		= amba_vtouch_probe,
	.remove		= __devexit_p(amba_vtouch_remove),
	.driver		= {
		.name	= "amba_vtouch",
		.owner	= THIS_MODULE,
	},
};

static int __init amba_vtouch_init(void)
{
	return platform_driver_register(&amba_vtouch_driver);
}

static void __exit amba_vtouch_exit(void)
{
	platform_driver_unregister(&amba_vtouch_driver);
}

module_init(amba_vtouch_init);
module_exit(amba_vtouch_exit);

MODULE_AUTHOR("Keny Huang <skhuang@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Virtual TouchScreen Driver");
MODULE_LICENSE("GPL");
