/*
 * drivers/input/touchscreen/amba_vbutton.c
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
#include <linux/version.h>

#include <linux/aipc/i_button.h>
#include <linux/aipc/lk_button.h>

#define AMBA_VBUTTON_VERSION (((LINUX_VERSION_CODE)<<8)|0x01)

#define DEBUG_AMBA_VBUTTON 0
#if DEBUG_AMBA_VBUTTON
#define dbgmsg printk
#else
#define dbgmsg(...) //printk
#endif

struct amba_vbutton {
	struct input_dev *input_dev;
	char phys[32];
	int irq;
};

static struct amba_vbutton *amba_button_dev = NULL;

int amba_vbutton_key_pressed(int key_code)
{
	if((amba_button_dev==NULL) || (amba_button_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d\n",__func__, key_code);

	input_report_key(amba_button_dev->input_dev, key_code, 1);

	return 0;
}
EXPORT_SYMBOL(amba_vbutton_key_pressed);

int amba_vbutton_key_released(int key_code)
{
	if((amba_button_dev==NULL) || (amba_button_dev->input_dev==NULL)){
		return -1;
	}
	dbgmsg("===> %s, %d\n",__func__, key_code);

	input_report_key(amba_button_dev->input_dev, key_code, 0);

	return 0;
}
EXPORT_SYMBOL(amba_vbutton_key_released);

/*
 * The functions for inserting/removing us as a module.
 */

static int __devinit amba_vbutton_probe(struct platform_device *pdev)
{
	struct input_dev	*input_dev;
	int		err = 0;

		
	if(amba_button_dev!=NULL){
		dev_err(&pdev->dev, "Amba_vbutton already exists. Skip operation!\n");
		return 0;
	}

	/* Check if touch device valid via IPC */
	err = ipc_ibutton_is_module_valid(AMBA_VBUTTON_VERSION);
	if(err<0){
		dev_err(&pdev->dev, "Amba_vbutton module does not exist. Skip operation!\n");
		return -EINVAL;
	}
	
	/* Allocate memory for device */
	amba_button_dev = kzalloc(sizeof(struct amba_vbutton), GFP_KERNEL);
	if (!amba_button_dev) {
		dev_err(&pdev->dev, "failed to allocate memory.\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, amba_button_dev);

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "failed to allocate input device.\n");
		err = -EBUSY;
		goto err_free_mem;
	}

/*	amba_button_dev->irq = platform_get_irq(pdev, 0);
	if (amba_button_dev->irq < 0) {
		dev_err(&pdev->dev, "no irq ID is designated.\n");
		err = -ENODEV;
		goto err_free_dev;
	}

	err = request_irq(amba_button_dev->irq, amba_vbutton_interrupt, IRQF_DISABLED,
			pdev->dev.driver->name, amba_button_dev);
	if (err) {
		dev_err(&pdev->dev, "failed to allocate irq.\n");
		goto err_unmap_regs;
	}*/

	amba_button_dev->input_dev = input_dev;

	snprintf(amba_button_dev->phys, sizeof(amba_button_dev->phys),
		 "%s/input0", dev_name(&pdev->dev));

	input_dev->name = "Ambarella virtual button controller";
	input_dev->phys = amba_button_dev->phys;
	input_dev->dev.parent = &pdev->dev;
	
	input_dev->id.bustype = BUS_HOST;
	set_bit(EV_KEY, input_dev->evbit);
//enable all key reporting
{ 	
	int j=0;
	for (j = 0; j < KEY_CNT; j++) {
		set_bit(j, input_dev->keybit);
	}
}
			
	/* All went ok, so register to the input system */
	err = input_register_device(input_dev);
	if (err){
		dev_err(&pdev->dev, "failed to input_register_device amba_vbutton.\n");
		goto err_fail;
	}
	dev_err(&pdev->dev, "probe amba_vbutton to [%s] done.\n",input_dev->phys);

	return 0;

err_fail:
//	free_irq(amba_button_dev->irq, amba_button_dev);
//err_free_dev:
	input_free_device(amba_button_dev->input_dev);
err_free_mem:
	kfree(amba_button_dev);
	amba_button_dev=NULL;
	return err;
}

static int __devexit amba_vbutton_remove(struct platform_device *pdev)
{
	struct amba_vbutton *t_dev = dev_get_drvdata(&pdev->dev);

	if(t_dev!=amba_button_dev){
		dev_err(&pdev->dev, "%s: Invalid amba_button_dev. Skip operation!\n",__func__);
		return 0;
	}
	
//	free_irq(amba_button_dev->irq, amba_button_dev);

	input_unregister_device(amba_button_dev->input_dev);

	kfree(amba_button_dev);
	amba_button_dev=NULL;

	return 0;
}

struct platform_device amba_vbutton = {
	.name			= "amba_vbutton",
	.id			= -1,
	.resource		= NULL,
	.num_resources		= 0,
	.dev			= {
		.platform_data		= NULL,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
EXPORT_SYMBOL(amba_vbutton);

static struct platform_driver amba_vbutton_driver = {
	.probe		= amba_vbutton_probe,
	.remove		= __devexit_p(amba_vbutton_remove),
	.driver		= {
		.name	= "amba_vbutton",
		.owner	= THIS_MODULE,
	},
};

static int __init amba_vbutton_init(void)
{
	return platform_driver_register(&amba_vbutton_driver);
}

static void __exit amba_vbutton_exit(void)
{
	platform_driver_unregister(&amba_vbutton_driver);
}

module_init(amba_vbutton_init);
module_exit(amba_vbutton_exit);

MODULE_AUTHOR("Keny Huang <skhuang@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Virtual Button Driver");
MODULE_LICENSE("GPL");
