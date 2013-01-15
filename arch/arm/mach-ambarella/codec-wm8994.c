/*
 * arch/arm/mach-ambarella/codec-wm8994.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * History:
 *	2011/03/28 - [Cao Rongrong] Created file
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/gpio.h>
#include <mach/init.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/regulator/fixed.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#define WM8994_LDO2ENA	GPIO(126)

static struct regulator_consumer_supply wm8994_avdd1_supply = {
	.dev_name	= "0-001a",
	.supply		= "AVDD1",
};

static struct regulator_consumer_supply wm8994_dcvdd_supply = {
	.dev_name	= "0-001a",
	.supply		= "DCVDD",
};

static struct regulator_init_data wm8994_ldo1_data = {
	.constraints	= {
		.name		= "AVDD1_3.0V",
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_avdd1_supply,
};

static struct regulator_init_data wm8994_ldo2_data = {
	.constraints	= {
		.name		= "DCVDD_1.0V",
		.always_on	= 1,

	},
	.supply_regulator	= "VDD_AUD_0.9V_3.3V",
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &wm8994_dcvdd_supply,
};

static struct wm8994_pdata ambarella_wm8994_pdata = {
	/* configure gpio1 function: GPIO */
	.gpio_defaults[0]	= 0x0001,
	/* configure gpio2 function: GPIO (disable pull-down) */
	.gpio_defaults[1]	= 0x8101,
	/* configure gpio3/4/5/7 function for AIF2 BT */
	.gpio_defaults[2]	= 0x8100,
	.gpio_defaults[3]	= 0x8100,
	.gpio_defaults[4]	= 0x8100,
	.gpio_defaults[6]	= 0x0100,
	/* configure gpio8/9/10/11 function for AIF3 3G */
	.gpio_defaults[7]	= 0,	//0x8100,
	.gpio_defaults[8]	= 0,	//0x0100,
	.gpio_defaults[9]	= 0,	//0x8100,
	.gpio_defaults[10]	= 0,	//0x8100,
	.ldo[0]			= { GPIO(125), NULL, &wm8994_ldo1_data },
	.ldo[1]			= { 0, NULL, &wm8994_ldo2_data },
	.lineout1_diff		= 1,
	.micbias1_lvl		= 1,
	.micbias2_lvl		= 1,
};

/* Fixed voltage regulators */

static struct regulator_consumer_supply wm8994_fixed_voltage0_supplies[] = {
	{
		.dev_name	= "0-001a",
		.supply		= "AVDD2",
	}, {
		.dev_name	= "0-001a",
		.supply		= "CPVDD",
	},
};

static struct regulator_consumer_supply wm8994_fixed_voltage1_supplies[] = {
	{
		.dev_name	= "0-001a",
		.supply		= "SPKVDD1",
	}, {
		.dev_name	= "0-001a",
		.supply		= "SPKVDD2",
	},
};

static struct regulator_consumer_supply wm8994_fixed_voltage2_supplies[] = {
	{
		.dev_name	= "0-001a",
		.supply		= "DBVDD",
	},
};

static struct regulator_init_data wm8994_fixed_voltage0_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage0_supplies),
	.consumer_supplies	= wm8994_fixed_voltage0_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage1_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage1_supplies),
	.consumer_supplies	= wm8994_fixed_voltage1_supplies,
};

static struct regulator_init_data wm8994_fixed_voltage2_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.supply_regulator	= "VSYS_AUD",
	.num_consumer_supplies	= ARRAY_SIZE(wm8994_fixed_voltage2_supplies),
	.consumer_supplies	= wm8994_fixed_voltage2_supplies,
};

static struct fixed_voltage_config wm8994_fixed_voltage0_config = {
	.supply_name	= "V1P8_AUD",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &wm8994_fixed_voltage0_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage1_config = {
	.supply_name		= "VSYS_AUD",
	.microvolts		= 5000000,
	.gpio			= GPIO(155),
	.startup_delay		= 1000,
	.enabled_at_boot	= true,
	.enable_high		= true,
	.init_data		= &wm8994_fixed_voltage1_init_data,
};

static struct fixed_voltage_config wm8994_fixed_voltage2_config = {
	.supply_name		= "V_D3P15",
	.microvolts		= 3150000,
	.gpio			= -EINVAL,
	.init_data		= &wm8994_fixed_voltage2_init_data,
};

static struct platform_device wm8994_fixed_voltage0 = {
	.name		= "reg-fixed-voltage",
	.id		= 0,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage0_config,
	},
};

static struct platform_device wm8994_fixed_voltage1 = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage1_config,
	},
};

static struct platform_device wm8994_fixed_voltage2 = {
	.name		= "reg-fixed-voltage",
	.id		= 2,
	.dev		= {
		.platform_data	= &wm8994_fixed_voltage2_config,
	},
};

static struct platform_device *wm8994_fixed_voltage_devices[] = {
	&wm8994_fixed_voltage0,
	&wm8994_fixed_voltage1,
	&wm8994_fixed_voltage2,
};

static struct i2c_board_info ambarella_wm8994_board_info = {
	.type		= "wm8994",
	.addr		= 0x1a,
	.platform_data	= &ambarella_wm8994_pdata,
};


int ambarella_init_wm8994(void)
{
	int rval = 0;

	rval = gpio_request(WM8994_LDO2ENA, "WM8994 LDO2 enable");
	if (rval < 0) {
		printk("Failed to get ldo2 GPIO: %d\n", rval);
		goto gpio_err;
	}

	rval = gpio_direction_output(WM8994_LDO2ENA, 1);
	if (rval < 0) {
		printk("Failed to set ldo2 GPIO up: %d\n", rval);
		goto gpio_err;
	}

	/* Delay to wait for ldo stable */
	msleep(150);

	platform_add_devices(wm8994_fixed_voltage_devices,
			ARRAY_SIZE(wm8994_fixed_voltage_devices));

	rval = i2c_register_board_info(0, &ambarella_wm8994_board_info, 1);
	if (rval < 0) {
		pr_err("%s: failed to register wm8994\n", __func__);
		return rval;
	}

gpio_err:
	return rval;
}

