/*
 * arch/arm/mach-ambarella/init-durian.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/board.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/i2c.h>
#include <linux/i2c/ak4183.h>
#include <linux/i2c/cy8ctmg.h>
#include <linux/i2c/pca953x.h>

#include <sound/ak4642_amb.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <linux/input.h>
#include <plat/ambinput.h>

#include "board-device.h"

/* ==========================================================================*/
#include <linux/pda_power.h>
static int ambarella_power_is_ac_online(void)
{
	return 1;
}

static struct pda_power_pdata  ambarella_power_supply_info = {
	.is_ac_online    = ambarella_power_is_ac_online,
};

static struct platform_device ambarella_power_supply = {
	.name = "pda-power",
	.id   = -1,
	.dev  = {
		.platform_data = &ambarella_power_supply_info,
	},
};

/* ==========================================================================*/
static struct platform_device *ambarella_devices[] __initdata = {
	&ambarella_adc0,
	&ambarella_crypto,
	&ambarella_eth0,
	&ambarella_fb0,
	&ambarella_fb1,
	&ambarella_i2s0,
	&ambarella_pcm0,
	&ambarella_dummy_codec0,
	&ambarella_idc0,
	&ambarella_idc1,
	&ambarella_i2cmux,
	&ambarella_ir0,
	&ambarella_rtc0,
	&ambarella_sd0,
	&ambarella_spi0,
	&ambarella_spi1,
	&ambarella_uart,
	&ambarella_uart1,
	&ambarella_udc0,
	&ambarella_wdt0,
	&ambarella_pwm_platform_device0,
	&ambarella_pwm_platform_device1,
	&ambarella_pwm_platform_device2,
	&ambarella_pwm_platform_device3,
	&ambarella_pwm_platform_device4,
	&ambarella_power_supply,
	&ambarella_fsg_device0,
	&ambarella_usb_device0,
};

/* ==========================================================================*/
static struct spi_board_info ambarella_spi_devices[] = {
	{
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 0,
	},
	{
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 1,
	},
	{
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 2,
	},
	{
		.modalias	= "spidev",
		.bus_num	= 0,
		.chip_select	= 3,
	},
	{
		.modalias	= "spidev",
		.bus_num	= 1,
		.chip_select	= 0,
	}
};

/* ==========================================================================*/
static struct ambarella_key_table durian_keymap[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,0,	2,	1000,	1023}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VOLUMEUP,0,	2,	945,	975}}},		//sw9: VOLUME_UP
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VOLUMEDOWN,0,	2,	900,	930}}},		//sw8: VOLUME_DOWN
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_POWER,	0,	2,	835,	885}}},		//sw7: POWER
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_SEND,	0,	2,	795,	845}}},		//sw6: CALL
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_HOME,	0,	2,	590,	640}}},		//sw5: HOME
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_MENU,	0,	2,	700,	750}}},		//sw4: MENU
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_BACK,	0,	2,	445,	495}}},		//sw3: BACK
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VIDEO,	0,	2,	270,	320}}},		//sb2: FOCUS
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_CAMERA,	0,	2,	0,	50}}},		//sb1: CAMERA

	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_POWER,	0,	1,	GPIO(85),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},

	{AMBINPUT_END}
};

static struct ambarella_key_table durian_keymap_v0b[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,0,	2,	1000,	1023}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VOLUMEUP,0,	2,	850,	910}}},		//sw9: VOLUME_UP
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VOLUMEDOWN,0,	2,	770,	830}}},		//sw8: VOLUME_DOWN
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_SEND,	0,	2,	670,	730}}},		//sw7: CALL
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_SEARCH,	0,	2,	610,	670}}},		//sw6: SEARCH
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_HOME,	0,	2,	490,	550}}},		//sw4: HOME
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_MENU,	0,	2,	360,	420}}},		//sw5: MENU
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_BACK,	0,	2,	240,	300}}},		//sw3: BACK
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VIDEO,	0,	2,	120,	180}}},		//sb2: FOCUS
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_CAMERA,	0,	2,	0,	50}}},		//sb1: CAMERA

	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_POWER,	0,	1,	GPIO(28),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},

	{AMBINPUT_END}
};

static struct ambarella_input_board_info durian_board_input_info = {
	.pkeymap		= durian_keymap,
	.pinput_dev		= NULL,
	.pdev			= NULL,

	.abx_max_x		= 4095,
	.abx_max_y		= 4095,
	.abx_max_pressure	= 4095,
	.abx_max_width		= 16,
};

static struct platform_device durian_board_input = {
	.name		= "ambarella-input",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &durian_board_input_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

/* ==========================================================================*/
static struct pca953x_platform_data durian_board_ext_gpio0 = {
	.gpio_base	= EXT_GPIO(0),
};

static struct i2c_board_info durian_board_ext_gpio_info = {
	.type		= "pca9557",
	.addr		= 0x1f,
	.platform_data	= &durian_board_ext_gpio0,
};

/* ==========================================================================*/
#define AK4642_RESET_PIN	102
#define AK4642_RESET_DELAY	1

static struct ak4642_platform_data durian_ak4642_pdata = {
	.rst_pin	= AK4642_RESET_PIN,
	.rst_delay	= AK4642_RESET_DELAY,
};

static struct i2c_board_info ambarella_ak4642_board_info = {
	I2C_BOARD_INFO("ak4642", 0x12),
	.platform_data	= &durian_ak4642_pdata,
};

/* ==========================================================================*/
static void __init ambarella_init_durian(void)
{
	int					i;

	ambarella_init_machine("Durian");

	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		durian_board_input_info.pkeymap = durian_keymap_v0b;
	}

	/* Config Board*/
	ambarella_board_generic.power_detect.irq_gpio = GPIO(22);
	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_board_generic.power_detect.irq_line = gpio_to_irq(28);
	} else {
		ambarella_board_generic.power_detect.irq_line = gpio_to_irq(22);
	}
	ambarella_board_generic.power_detect.irq_type = IRQF_TRIGGER_FALLING;
	ambarella_board_generic.power_detect.irq_gpio_val = GPIO_LOW;
	ambarella_board_generic.power_detect.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.debug_led0.gpio_id = GPIO(31);
	ambarella_board_generic.debug_led0.active_level = GPIO_LOW;
	ambarella_board_generic.debug_led0.active_delay = 0;

	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_board_generic.debug_switch.gpio_id = GPIO(26);
		ambarella_board_generic.debug_switch.active_level = GPIO_LOW;
		ambarella_board_generic.debug_switch.active_delay = 0;
	}

	ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(12);
	ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(12);
	ambarella_board_generic.touch_panel_irq.irq_type = IRQF_TRIGGER_FALLING;
	ambarella_board_generic.touch_panel_irq.irq_gpio_val = GPIO_LOW;
	ambarella_board_generic.touch_panel_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.lcd_power.gpio_id = GPIO(38);
	ambarella_board_generic.lcd_power.active_level = GPIO_HIGH;
	ambarella_board_generic.lcd_power.active_delay = 1;

	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_board_generic.lcd_reset.gpio_id = GPIO(22);
	} else {
		ambarella_board_generic.lcd_reset.gpio_id = GPIO(28);
	}
	ambarella_board_generic.lcd_reset.active_level = GPIO_LOW;
	ambarella_board_generic.lcd_reset.active_delay = 1;

	ambarella_board_generic.lcd_backlight.gpio_id = GPIO(16);
	ambarella_board_generic.lcd_backlight.active_level = GPIO_HIGH;
	ambarella_board_generic.lcd_backlight.active_delay = 1;

	ambarella_board_generic.lcd_spi_hw.bus_id = 0;
	ambarella_board_generic.lcd_spi_hw.cs_id = 1;

	ambarella_board_generic.vin_vsync.irq_gpio = GPIO(95);
	ambarella_board_generic.vin_vsync.irq_line = gpio_to_irq(95);
	ambarella_board_generic.vin_vsync.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.vin_vsync.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.vin_vsync.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.vin_power.gpio_id = EXT_GPIO(0);
	ambarella_board_generic.vin_power.active_level = GPIO_HIGH;
	ambarella_board_generic.vin_power.active_delay = 1;

	ambarella_board_generic.vin_reset.gpio_id = GPIO(7);
	ambarella_board_generic.vin_reset.active_level = GPIO_LOW;
	ambarella_board_generic.vin_reset.active_delay = 100;

	ambarella_board_generic.vin_strobe.irq_gpio = GPIO(21);
	ambarella_board_generic.vin_strobe.irq_line = gpio_to_irq(21);
	ambarella_board_generic.vin_strobe.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.vin_strobe.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.vin_strobe.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.flash_charge_ready.irq_gpio = GPIO(13);
	ambarella_board_generic.flash_charge_ready.irq_line = gpio_to_irq(13);
	ambarella_board_generic.flash_charge_ready.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.flash_charge_ready.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.flash_charge_ready.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.flash_trigger.gpio_id = GPIO(46);
	ambarella_board_generic.flash_trigger.active_level = GPIO_LOW;
	ambarella_board_generic.flash_trigger.active_delay = 1;

	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_board_generic.flash_enable.gpio_id = GPIO(30);
	} else {
		ambarella_board_generic.flash_enable.gpio_id = GPIO(82);
	}
	ambarella_board_generic.flash_enable.active_level = GPIO_LOW;
	ambarella_board_generic.flash_enable.active_delay = 1;

	ambarella_board_generic.avplug_detect.irq_gpio = GPIO(11);
	ambarella_board_generic.avplug_detect.irq_line = gpio_to_irq(11);
	ambarella_board_generic.avplug_detect.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.avplug_detect.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.avplug_detect.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.hdmi_extpower.gpio_id = GPIO(20);
	ambarella_board_generic.hdmi_extpower.active_level = GPIO_HIGH;
	ambarella_board_generic.hdmi_extpower.active_delay = 1;

	ambarella_board_generic.gps_irq.irq_gpio = GPIO(24);
	ambarella_board_generic.gps_irq.irq_line = gpio_to_irq(24);
	ambarella_board_generic.gps_irq.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.gps_irq.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.gps_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.gps_power.gpio_id = EXT_GPIO(7);
	ambarella_board_generic.gps_power.active_level = GPIO_HIGH;
	ambarella_board_generic.gps_power.active_delay = 1;

	ambarella_board_generic.gps_reset.gpio_id = GPIO(23);
	ambarella_board_generic.gps_reset.active_level = GPIO_HIGH;
	ambarella_board_generic.gps_reset.active_delay = 1;

	ambarella_board_generic.gps_wakeup.gpio_id = GPIO(25);
	ambarella_board_generic.gps_wakeup.active_level = GPIO_HIGH;
	ambarella_board_generic.gps_wakeup.active_delay = 1;

	if (AMBARELLA_BOARD_REV(system_rev) < 2) {
		ambarella_board_generic.lens_power.gpio_id = GPIO(26);
		ambarella_board_generic.lens_power.active_level = GPIO_HIGH;
		ambarella_board_generic.lens_power.active_delay = 1;
	}

	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_board_generic.gyro_irq.irq_gpio = GPIO(93);
		ambarella_board_generic.gyro_irq.irq_line = gpio_to_irq(93);
		ambarella_board_generic.gyro_irq.irq_type = IRQF_TRIGGER_RISING;
		ambarella_board_generic.gyro_irq.irq_gpio_val = GPIO_HIGH;
		ambarella_board_generic.gyro_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
	}

	ambarella_board_generic.gyro_power.gpio_id = EXT_GPIO(2);
	ambarella_board_generic.gyro_power.active_level = GPIO_HIGH;
	ambarella_board_generic.gyro_power.active_delay = 1;

	ambarella_board_generic.gyro_hps.gpio_id = GPIO(27);
	ambarella_board_generic.gyro_hps.active_level = GPIO_HIGH;
	ambarella_board_generic.gyro_hps.active_delay = 1;

	ambarella_board_generic.fm_power.gpio_id = EXT_GPIO(4);
	ambarella_board_generic.fm_power.active_level = GPIO_HIGH;
	ambarella_board_generic.fm_power.active_delay = 1;

	ambarella_board_generic.gsensor_irq.irq_gpio = GPIO(29);
	ambarella_board_generic.gsensor_irq.irq_line = gpio_to_irq(29);
	ambarella_board_generic.gsensor_irq.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.gsensor_irq.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.gsensor_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.gsensor_power.gpio_id = EXT_GPIO(5);
	ambarella_board_generic.gsensor_power.active_level = GPIO_HIGH;
	ambarella_board_generic.gsensor_power.active_delay = 1;

	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_board_generic.bb_irq.irq_gpio = GPIO(45);
		ambarella_board_generic.bb_irq.irq_line = gpio_to_irq(45);
		ambarella_board_generic.bb_irq.irq_type = IRQF_TRIGGER_RISING;
		ambarella_board_generic.bb_irq.irq_gpio_val = GPIO_HIGH;
		ambarella_board_generic.bb_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

		ambarella_board_generic.bb_power.gpio_id = GPIO(85);
		ambarella_board_generic.bb_power.active_level = GPIO_LOW;
		ambarella_board_generic.bb_power.active_delay = 1;

		ambarella_board_generic.bb_reset.gpio_id = GPIO(94);
		ambarella_board_generic.bb_reset.active_level = GPIO_LOW;
		ambarella_board_generic.bb_reset.active_delay = 1;

		ambarella_board_generic.bb_en.gpio_id = GPIO(84);
		ambarella_board_generic.bb_en.active_level = GPIO_HIGH;
		ambarella_board_generic.bb_en.active_delay = 1;

		ambarella_board_generic.bb_switch.gpio_id = GPIO(92);
		ambarella_board_generic.bb_switch.active_level = GPIO_HIGH;
		ambarella_board_generic.bb_switch.active_delay = 1;
	}

	/* Config SD*/
	fio_default_owner = SELECT_FIO_SDIO;
	ambarella_platform_sd_controller0.clk_limit = 25000000;
	ambarella_platform_sd_controller0.slot[0].cd_delay = 100;
	ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
	ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_64KB;
	if (AMBARELLA_BOARD_REV(system_rev) >= 2) {
		ambarella_platform_sd_controller0.slot[0].ext_power.gpio_id = EXT_GPIO(3);
		ambarella_platform_sd_controller0.slot[0].ext_power.active_level = GPIO_HIGH;
		ambarella_platform_sd_controller0.slot[0].ext_power.active_delay = 300;
	}
	/* Disable Power control, use /sys/class/gpio to control them.
	ambarella_platform_sd_controller0.slot[1].ext_power.gpio_id = EXT_GPIO(1);
	ambarella_platform_sd_controller0.slot[1].ext_power.active_level = GPIO_HIGH;
	ambarella_platform_sd_controller0.slot[1].ext_power.active_delay = 300;
	ambarella_platform_sd_controller0.slot[1].ext_reset.gpio_id = GPIO(54);
	ambarella_platform_sd_controller0.slot[1].ext_reset.active_level = GPIO_LOW;
	ambarella_platform_sd_controller0.slot[1].ext_reset.active_delay = 100;
	*/
	ambarella_platform_sd_controller0.slot[1].cd_delay = 100;
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio = GPIO(75);
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_line = gpio_to_irq(75);
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio_val = GPIO_HIGH;
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
	ambarella_platform_sd_controller0.slot[1].gpio_wp.gpio_id = GPIO(76);
	ambarella_platform_sd_controller0.slot[1].gpio_wp.active_level = GPIO_LOW;
	ambarella_platform_sd_controller0.slot[1].gpio_wp.active_delay = 0;

	/* Register devices*/
	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	spi_register_board_info(ambarella_spi_devices,
		ARRAY_SIZE(ambarella_spi_devices));

	i2c_register_board_info(0, &ambarella_ak4642_board_info, 1);

	ambarella_chacha_mt4d_board_info.irq =
		ambarella_board_generic.touch_panel_irq.irq_line;
	i2c_register_board_info(2, &ambarella_chacha_mt4d_board_info, 1);

	i2c_register_board_info(0, ambarella_board_vin_infos,
		ARRAY_SIZE(ambarella_board_vin_infos));
	i2c_register_board_info(1, &ambarella_board_hdmi_info, 1);

	i2c_register_board_info(2, &durian_board_ext_gpio_info, 1);

	platform_device_register(&durian_board_input);
}

/* ==========================================================================*/
MACHINE_START(DURIAN, "Durian")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_durian,
MACHINE_END

