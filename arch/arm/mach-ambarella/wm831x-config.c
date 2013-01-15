/*
 * arch/arm/mach-ambarella/Wm831x-regulator-config.c
 *
 * Author: Bingliang-Hu <blhu@ambarella.com>
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
#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/board.h>

#include <linux/regulator/machine.h>

#include <linux/mfd/wm831x/core.h>
#include <linux/mfd/wm831x/pdata.h>
#include <linux/mfd/wm831x/regulator.h>

/* DCDC1: iOne_VDDAX for Cortex and 3D */
static struct regulator_consumer_supply dcdc1_consumers[] = {
	{
		.supply = "cpu_vcc",
	},
};

static struct regulator_init_data elephant_wm8310_dcdc1_data = {
	.constraints = {
		.name = "VDD_AXI_1.0V_1.4V",
		.min_uV = 1000000,
		.max_uV = 1400000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = REGULATOR_MODE_STANDBY,
		},
		.initial_state = PM_SUSPEND_MEM,
		.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(dcdc1_consumers),
	 .consumer_supplies = dcdc1_consumers,
};

static struct regulator_consumer_supply dcdc2_consumers[] = {
	{
		.supply = "ddr3_vcc",
	},
};
/* DCDC2 iOne_D1P5 for DDR3 */
static struct regulator_init_data elephant_wm8310_dcdc2_data = {
	.constraints = {
		.name = "VDDQ_i1_0.6V_1.8",
		.min_uV = 600000,
		.max_uV = 1800000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = REGULATOR_MODE_STANDBY,
		},
		.initial_state = PM_SUSPEND_MEM,
		.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(dcdc2_consumers),
	 .consumer_supplies = dcdc2_consumers,
};
static struct regulator_consumer_supply dcdc3_consumers[] = {
	{
		.supply = "gen_vcc",
	},
	{
		.supply = "lp_vcc",
	},
};
/* DCDC3 iOne_D3P15 for A3P15 or D3P15 */
static struct regulator_init_data elephant_wm8310_dcdc3_data = {
	.constraints = {
		.name = "VDD33_LP_AND_GEN_3.15V",
		.min_uV = 2800000,
		.max_uV = 3400000,
		.apply_uV = 0,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = REGULATOR_MODE_STANDBY,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(dcdc3_consumers),
	.consumer_supplies = dcdc3_consumers,
};

static struct regulator_consumer_supply dcdc4_consumers[] = {
	{
		.supply = "lcd_vcc",
	},
};

static struct regulator_init_data elephant_wm8310_dcdc4_data = {
	.constraints = {
		.name = "VDD_LCD_6.5V_30V",
		.min_uV = 6500000,
		.max_uV = 30000000,
		.apply_uV = 0,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
		},
		//.initial_state = PM_SUSPEND_MAX,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(dcdc4_consumers),
	 .consumer_supplies = dcdc4_consumers,
};

static struct regulator_consumer_supply ldo1_consumers[] = {
	{
		.supply = "mipi_vcc",
	},
};
/* LDO1 MIPI_PHY */
static struct regulator_init_data elephant_wm8310_ldo1_data = {
	.constraints = {
		.name = "VDD_MIPI_PHY_0.9V_3.3V",
		.min_uV = 900000,
		.max_uV = 3300000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO1_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo1_consumers),
	 .consumer_supplies = ldo1_consumers,
};
static struct regulator_consumer_supply ldo2_consumers[] = {
	{
		.supply = "sensor_vcc",
	},
};
/* LDO2 SEN_VDD */
static struct regulator_init_data elephant_wm8310_ldo2_data = {
	.constraints = {
		.name = "VDD_SEN_0.9V_3.3V",
		.min_uV = 900000,
		.max_uV = 3300000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO2_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo2_consumers),
	 .consumer_supplies = ldo2_consumers,
};
static struct regulator_consumer_supply ldo3_consumers[] = {
	{
		.supply = "audio_vcc",
	},
};
/* LDO3 Audio codec power */
static struct regulator_init_data elephant_wm8310_ldo3_data = {
	.constraints = {
		.name = "VDD_AUD_0.9V_3.3V",
		.min_uV = 900000,
		.max_uV = 3300000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO3_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo3_consumers),
	 .consumer_supplies = ldo3_consumers,
};

static struct regulator_consumer_supply ldo4_consumers[] = {
	{
		.supply = "gyro_vcc",
	},
};
/* LDO4 gyro sensor */
static struct regulator_init_data elephant_wm8310_ldo4_data = {
	.constraints = {
		.name = "VDD_GY_0.9V_3.3V",
		.min_uV = 900000,
		.max_uV = 3300000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO4_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo4_consumers),
	 .consumer_supplies = ldo4_consumers,
};
static struct regulator_consumer_supply ldo5_consumers[] = {
	{
		.supply = "sdxc_vcc",
	},
};
/* LDO5 VDD33_SDXC */
static struct regulator_init_data elephant_wm8310_ldo5_data = {
	.constraints = {
		.name = "VDD33_SDXC_0.9V_3.3V",
		/* can not get 3.15 but 3.1 or 3.2 */
		.min_uV = 900000,
		.max_uV = 3300000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO5_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo5_consumers),
	 .consumer_supplies = ldo5_consumers,
};
static struct regulator_consumer_supply ldo6_consumers[] = {
	{
		.supply = "gsensor_vcc",
	},
};
/* LDO6 Gsensor */
static struct regulator_init_data elephant_wm8310_ldo6_data = {
	.constraints = {
		.name = "VDD_G_0.9V_3.3V",
		.min_uV = 900000,
		.max_uV = 3300000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO6_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo6_consumers),
	 .consumer_supplies = ldo6_consumers,
};
static struct regulator_consumer_supply ldo7_consumers[] = {
	{
		.supply = "analog_vcc",
	},
};
/* LDO7 VDDA_25_XX */
static struct regulator_init_data elephant_wm8310_ldo7_data = {
	.constraints = {
		.name = "VDDA_25_XXX_1V_3.5V",
		.min_uV = 1000000,
		.max_uV = 3500000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO7_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo7_consumers),
	 .consumer_supplies = ldo7_consumers,
};
static struct regulator_consumer_supply ldo8_consumers[] = {
	{
		.supply = "imagesen_vcc",
	},
};
/* LDO8 Image sensor */
static struct regulator_init_data elephant_wm8310_ldo8_data = {
	.constraints = {
		.name = "VDD_IMG_SEN_1V_3.5",
		.min_uV = 1000000,
		.max_uV = 3500000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO8_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo8_consumers),
	 .consumer_supplies = ldo8_consumers,
};
static struct regulator_consumer_supply ldo9_consumers[] = {
	{
		.supply = "bt_vcc",
	},
};
/* LDO9 VDD_BT */
static struct regulator_init_data elephant_wm8310_ldo9_data = {
	.constraints = {
		.name = "VDD_BT_1V_3.5V",
		.min_uV = 1000000,
		.max_uV = 3500000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO9_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo9_consumers),
	 .consumer_supplies = ldo9_consumers,
};
static struct regulator_consumer_supply ldo10_consumers[] = {
	{
		.supply = "lcd_ctl_vcc",
	},
};
/* LDO10 VDD_LCD */
static struct regulator_init_data elephant_wm8310_ldo10_data = {
	.constraints = {
		.name = "VDD_LCD_1V_3.5V",
		.min_uV = 1000000,
		.max_uV = 3500000,
		.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_MODE,
		.valid_modes_mask = REGULATOR_MODE_STANDBY | REGULATOR_MODE_NORMAL,
		.state_mem = {
			.disabled = 1,
			.mode = WM831X_LDO10_SLP_MODE,
		},
		.initial_state = PM_SUSPEND_MEM,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(ldo10_consumers),
	 .consumer_supplies = ldo10_consumers,
};

static struct regulator_consumer_supply lsink1_consumers[] = {
	{
		.supply = "lcd_bl_sink",
	},
};
/* ISINK1 backlight */
static struct regulator_init_data elephant_wm8310_isink1_data = {
	.constraints = {
		.name = "VDD_LCD_BL_20mA_28V",
		.min_uA = 19484,
		.max_uA = 19484,
		//.apply_uV = 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem = {
			.disabled = 1,
		},
		//.initial_state = PM_SUSPEND_MAX,
		//.always_on = 1,
		.boot_on = 1,
	},
	 .num_consumer_supplies = ARRAY_SIZE(lsink1_consumers),
	 .consumer_supplies = lsink1_consumers,
};
static struct wm831x_backlight_pdata elephant_wm8310_backlight_pdata = {
	.isink = 1,
	/* 20*2^(13.25) = 19484 */
	.max_uA = 19484,
};


static struct wm831x_battery_pdata elephant_wm8310_battery_pdata = {
	.enable = 1,
	.fast_enable	 = 1,
//	.off_mask = 1,
	.trickle_ilim = 50,
	.vsel = 4200,
	.eoc_iterm = 20,
	.fast_ilim = 450,
	.timeout = 360,
	.enable_fet = 1,
};

#define WM8310_GPIO_START EXT_GPIO(0)
static struct gpio elephant_wm8310_gpios[] = {
/* pmic gpio 0..5 can be controlled with OTP data */
#if 0
		{WM8310_GPIO_START + 0, GPIOF_OUT_INIT_HIGH, "VDD_EN1"},
		{WM8310_GPIO_START + 1, GPIOF_OUT_INIT_HIGH, "VDD_EN2"},
		{WM8310_GPIO_START + 2, GPIOF_OUT_INIT_HIGH, "VDD_EN3"},
		{WM8310_GPIO_START + 3, GPIOF_OUT_INIT_HIGH, "VDD_EN4"},
		{WM8310_GPIO_START + 4, GPIOF_OUT_INIT_HIGH, "VDD_EN5"},
		{WM8310_GPIO_START + 5, GPIOF_OUT_INIT_HIGH, "VDD_EN6"},
#endif
		{WM8310_GPIO_START + 6, GPIOF_OUT_INIT_HIGH, "VDD_EN7"},
		{WM8310_GPIO_START + 7, GPIOF_OUT_INIT_HIGH, "VDD_EN8"},
		{WM8310_GPIO_START + 8, GPIOF_OUT_INIT_HIGH, "VDD_EN9"},
};

static int elephant_wm8310_pre_init(struct wm831x *wm831x)
{
	int					ret = 0;
	if (ambarella_is_valid_gpio_irq(&ambarella_board_generic.pmic_irq)) {
		wm831x->irq = ambarella_board_generic.pmic_irq.irq_line;
		set_irq_wake(wm831x->irq, 1);
	} else {
		ret = -1;
	}

	return ret;
}

/* setup the gpio 1..9 (1 is the 1st one) as power supply by hw init/OTP */
/* we only check the gpio's high value */
static int elephant_wm8310_post_init(struct wm831x *wm831x)
{
	int i, ret = 0;
	if ((ret = gpio_request_array(elephant_wm8310_gpios, ARRAY_SIZE(elephant_wm8310_gpios)))) {
		printk("Error request gpio for wm8310 on iOne\n");
		return ret;
	};
	for (i = 0 ; i < ARRAY_SIZE(elephant_wm8310_gpios); i++) {
		gpio_set_value_cansleep(elephant_wm8310_gpios[i].gpio, 1);
		if (!gpio_get_value_cansleep(elephant_wm8310_gpios[i].gpio)) {
			printk("WARNING:wm8310 gpio[%d] can not be pulled high.\n",
				elephant_wm8310_gpios[i].gpio - WM8310_GPIO_START);
			/* we hope it will work anyway */
			//ret = -EINVAL;
		};
	}

	return ret;
}

struct wm831x_pdata elephant_wm8310_pdata = {
	.pre_init = elephant_wm8310_pre_init,
	.post_init = elephant_wm8310_post_init,
	/* CIFMODE is pulled up to enable wm8310 spi mode with GPIO */
	.irq_base = EXT_IRQ(0),

	.gpio_base = EXT_GPIO(0),
	.backlight = &elephant_wm8310_backlight_pdata,

	.battery = &elephant_wm8310_battery_pdata,

	.dcdc = {
		&elephant_wm8310_dcdc1_data, /* DCDC1 */
		&elephant_wm8310_dcdc2_data, /* DCDC2 */
		&elephant_wm8310_dcdc3_data, /* DCDC3 */
		&elephant_wm8310_dcdc4_data, /* DCDC4 */
	},
	.ldo = {
		&elephant_wm8310_ldo1_data,
		&elephant_wm8310_ldo2_data,
		&elephant_wm8310_ldo3_data,
		&elephant_wm8310_ldo4_data,
		&elephant_wm8310_ldo5_data,
		&elephant_wm8310_ldo6_data,
		&elephant_wm8310_ldo7_data,
		&elephant_wm8310_ldo8_data,
		&elephant_wm8310_ldo9_data,
		&elephant_wm8310_ldo10_data,
	},

	.isink = {
		&elephant_wm8310_isink1_data, /* ISINK1 */
	},
};


