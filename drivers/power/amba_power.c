/*
 * PMU driver for Wolfson Microelectronics ambapmic PMICs
 *
 * Copyright 2009 Wolfson Microelectronics PLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>

#include <linux/aipc/i_pmic.h>

//#include <linux/discharge_curve.h>

struct ambapmic_power {
	struct power_supply wall;
	struct power_supply usb;
	struct power_supply battery;
	struct timer_list battery_update_timer;
};

struct ambapmic_power *G_power=NULL;
#define DEBUG printk

//Auto Update timer is turned on as Default at WM831x.
//Turn it off for debug
#define ENABLE_AUTOUPDATE_FUNCTION 0

#if ENABLE_AUTOUPDATE_FUNCTION
#define	ANDROID_BAT_UPDATE_DELAY_MSEC	5000
static void battery_update_timer_func(unsigned long data)
{
	struct ambapmic_power *power = (struct ambapmic_power *)data;

	DEBUG("Update battery status\n");
	power_supply_changed(&power->battery);

	mod_timer(&power->battery_update_timer,
		  jiffies + msecs_to_jiffies(ANDROID_BAT_UPDATE_DELAY_MSEC));
}
#endif


/*********************************************************************
 *		WALL Power
 *********************************************************************/
static int ambapmic_wall_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;

	if(G_power==NULL){
		printk("%s: not initialized!\n",__func__);
		return -EINVAL;
	}
	
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = ipc_ipmic_get_prop_int(IPC_PMIC_MOD_WALL,psp);
		break;
	default:
		DEBUG("%s: unsupport props(%d)\n",__func__,psp);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property ambapmic_wall_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

/*********************************************************************
 *		USB Power
 *********************************************************************/
static int ambapmic_usb_get_prop(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	int ret = 0;

	if(G_power==NULL){
		printk("%s: not initialized!\n",__func__);
		return -EINVAL;
	}
	
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = ipc_ipmic_get_prop_int(IPC_PMIC_MOD_USB,psp);
		break;
	default:
		DEBUG("%s: unsupport props(%d)\n",__func__,psp);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property ambapmic_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

/*********************************************************************
 *		Battery properties
 *********************************************************************/

static int ambapmic_bat_get_prop(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	int ret = 0;

	if(G_power==NULL){
		printk("%s: not initialized!\n",__func__);
		return -EINVAL;
	}

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_CAPACITY:
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = ipc_ipmic_get_prop_int(IPC_PMIC_MOD_BAT,psp);
		break;
	default:
		DEBUG("%s: unsupport props(%d)\n",__func__,psp);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property ambapmic_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_TEMP,
};

int ambapmic_pwr_src_notify(unsigned int modules)
{
	DEBUG("Power source changed\n");

	if(modules&IPC_PMIC_MOD_BAT){
		DEBUG("Battery status changed\n");
		power_supply_changed(&G_power->battery);
	}
	if(modules&IPC_PMIC_MOD_USB){
		DEBUG("USB status changed\n");
		power_supply_changed(&G_power->usb);
	}
	if(modules&IPC_PMIC_MOD_WALL){
		DEBUG("AC status changed\n");
		power_supply_changed(&G_power->wall);
	}

	return 0;
}
EXPORT_SYMBOL(ambapmic_pwr_src_notify);

static __devinit int ambapmic_power_probe(struct platform_device *pdev)
{
	struct ambapmic_power *power=NULL;
	struct power_supply *usb=NULL;
	struct power_supply *battery=NULL;
	struct power_supply *wall=NULL;
	int ret;
	unsigned int mod;

	if(G_power!=NULL){
		printk("%s: Already probed!!\n",__func__);
		return 0;
	}

	mod=ipc_ipmic_get_modules(0);
	if(mod==0){
		printk("%s: No power_supply module available!!\n",__func__);
		return 0;
	}
	
	power = kzalloc(sizeof(struct ambapmic_power), GFP_KERNEL);
	if (power == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, power);
	G_power = power;
	
	if(mod&IPC_PMIC_MOD_WALL){
		DEBUG("%s: initialize AC!\n",__func__);
		ipc_ipmic_init_module(IPC_PMIC_MOD_WALL);
		
		wall = &power->wall;

		wall->name = "ambapmic-wall";
		wall->type = POWER_SUPPLY_TYPE_MAINS;
		wall->properties = ambapmic_wall_props;
		wall->num_properties = ARRAY_SIZE(ambapmic_wall_props);
		wall->get_property = ambapmic_wall_get_prop;
		ret = power_supply_register(&pdev->dev, wall);
		if (ret){
			printk("%s: initialize AC fail!\n",__func__);
			goto err_kmalloc;
		}
	}

	if(mod&IPC_PMIC_MOD_BAT){
		DEBUG("%s: initialize Battery!\n",__func__);
		ipc_ipmic_init_module(IPC_PMIC_MOD_BAT);
		
		battery = &power->battery;

		battery->name = "ambapmic-battery";
		battery->properties = ambapmic_bat_props;
		battery->num_properties = ARRAY_SIZE(ambapmic_bat_props);
		battery->get_property = ambapmic_bat_get_prop;
		battery->use_for_apm = 1;

#if ENABLE_AUTOUPDATE_FUNCTION
		setup_timer(&power->battery_update_timer, battery_update_timer_func, (long unsigned int)power);
		mod_timer(&power->battery_update_timer,
			  jiffies + msecs_to_jiffies(ANDROID_BAT_UPDATE_DELAY_MSEC));
#endif
		ret = power_supply_register(&pdev->dev, battery);
		if (ret){
			printk("%s: initialize Battery fail!\n",__func__);
			goto err_wall;
		}
	}

	if(mod&IPC_PMIC_MOD_USB){
		DEBUG("%s: initialize USB!\n",__func__);
		ipc_ipmic_init_module(IPC_PMIC_MOD_USB);
		
		usb = &power->usb;

		usb->name = "ambapmic-usb",
		usb->type = POWER_SUPPLY_TYPE_USB;
		usb->properties = ambapmic_usb_props;
		usb->num_properties = ARRAY_SIZE(ambapmic_usb_props);
		usb->get_property = ambapmic_usb_get_prop;
		ret = power_supply_register(&pdev->dev, usb);
		if (ret){
			printk("%s: initialize USB fail!\n",__func__);
			goto err_battery;
		}
	}
	
	
	return ret;

err_battery:
	power_supply_unregister(battery);
err_wall:
	power_supply_unregister(wall);
err_kmalloc:
	G_power = NULL;
	kfree(power);
	return ret;
}

static __devexit int ambapmic_power_remove(struct platform_device *pdev)
{
	struct ambapmic_power *ambapmic_power = platform_get_drvdata(pdev);

	del_timer_sync(&ambapmic_power->battery_update_timer);

	power_supply_unregister(&ambapmic_power->battery);
	power_supply_unregister(&ambapmic_power->wall);
	power_supply_unregister(&ambapmic_power->usb);
	G_power = NULL;
	kfree(ambapmic_power);
	return 0;
}

static struct platform_driver ambapmic_power_driver = {
	.probe = ambapmic_power_probe,
	.remove = __devexit_p(ambapmic_power_remove),
	.driver = {
		.name = "ambapmic-power",
	},
};

struct platform_device ambapmic_power = {
	.name			= "ambapmic-power",
	.id			= -1,
	.resource		= NULL,
	.num_resources		= 0,
	.dev			= {
		.platform_data		= NULL,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
EXPORT_SYMBOL(ambapmic_power);

static int __init ambapmic_power_init(void)
{
	return platform_driver_register(&ambapmic_power_driver);
}
module_init(ambapmic_power_init);

static void __exit ambapmic_power_exit(void)
{
	platform_driver_unregister(&ambapmic_power_driver);
}
module_exit(ambapmic_power_exit);

MODULE_DESCRIPTION("Ambarella virtual Power supply driver");
MODULE_AUTHOR("Ambarella <@ambarella.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ambapmic-power");
