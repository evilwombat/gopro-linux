/*
 * arch/arm/mach-ambarella/init-coconut.c
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
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/pda_power.h>
#include <sound/ak4642_amb.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <plat/ambinput.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/board.h>

#include "board-device.h"

/* ==========================================================================*/
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
static struct ambarella_key_table coconut_keymap[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,0,	0,	983,	1023}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {14,		0,	0,	880,	920,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_END,	0,	0,	780,	820,}}},	//DEL
	{AMBINPUT_ADC_KEY,	{.adc_key	= {127,		0,	0,	690,	730,}}},	//MENU
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_OK,	0,	0,	620,	640,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RIGHT,	0,	0,	490,	530,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_LEFT,	0,	0,	360,	400,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_DOWN,	0,	0,	250,	300,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_UP,	0,	0,	120,	160,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_ESC,	0,	0,	0,	40,}}},		//S1

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,0,	1,	983,	1023}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {228,		0,	1,	880,	920,}}},	//POUND
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_PHONE,	0,	1,	780,	820,}}},	//S13
	{AMBINPUT_ADC_KEY,	{.adc_key	= {227,		0,	1,	690,	730,}}},	//STAR
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RECORD,	0,	1,	620,	640,}}},	//S11
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_PLAY,	0,	1,	490,	530,}}},	//S10
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_POWER,	0,	1,	360,	400,}}},	//HOME	WAKE
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_VIDEO,	0,	1,	250,	300,}}},	//S15
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_HP,	0,	1,	120,	160,}}},	//FOCUS
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_CAMERA,	0,	1,	0,	40,}}},		//CAMERA

	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {125,		0,	0,	GPIO(13),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_POWER,	0,	1,	GPIO(11),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},

	{AMBINPUT_END}
};

static struct ambarella_input_board_info coconut_board_input_info = {
	.pkeymap		= coconut_keymap,
	.pinput_dev		= NULL,
	.pdev			= NULL,

	.abx_max_x		= 4095,
	.abx_max_y		= 4095,
	.abx_max_pressure	= 4095,
	.abx_max_width		= 16,
};

static struct platform_device coconut_board_input = {
	.name		= "ambarella-input",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &coconut_board_input_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

/* ==========================================================================*/
#define AK4642_RESET_PIN	12
#define AK4642_RESET_DELAY	1

static struct ak4642_platform_data coconut_ak4642_pdata = {
	.rst_pin	= AK4642_RESET_PIN,
	.rst_delay	= AK4642_RESET_DELAY,
};

static struct i2c_board_info ambarella_ak4642_board_info = {
	I2C_BOARD_INFO("ak4642", 0x12),
	.platform_data	= &coconut_ak4642_pdata,
};

/* ==========================================================================*/
static void __init ambarella_init_coconut(void)
{
	int					i;

	ambarella_init_machine("Coconut");

	/* Config Board*/
	ambarella_board_generic.power_detect.irq_gpio = GPIO(11);
	ambarella_board_generic.power_detect.irq_line = gpio_to_irq(11);
	ambarella_board_generic.power_detect.irq_type = IRQF_TRIGGER_FALLING;
	ambarella_board_generic.power_detect.irq_gpio_val = GPIO_LOW;
	ambarella_board_generic.power_detect.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.debug_led0.gpio_id = GPIO(83);
	ambarella_board_generic.debug_led0.active_level = GPIO_LOW;
	ambarella_board_generic.debug_led0.active_delay = 0;

	ambarella_board_generic.rs485.gpio_id = GPIO(31);
	ambarella_board_generic.rs485.active_level = GPIO_LOW;
	ambarella_board_generic.rs485.active_delay = 1;

#ifdef CONFIG_TOUCH_AMBARELLA_TM1510
	ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(85);
	ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(85);
#else
	ambarella_board_generic.touch_panel_irq.irq_gpio = GPIO(84);
	ambarella_board_generic.touch_panel_irq.irq_line = gpio_to_irq(84);
#endif
	ambarella_board_generic.touch_panel_irq.irq_type = IRQF_TRIGGER_FALLING;
	ambarella_board_generic.touch_panel_irq.irq_gpio_val = GPIO_LOW;
	ambarella_board_generic.touch_panel_irq.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	if (AMBARELLA_BOARD_REV(system_rev) > 10) {
		ambarella_board_generic.lcd_backlight.gpio_id = GPIO(85);
	} else {
		ambarella_board_generic.lcd_backlight.gpio_id = GPIO(45);
	}
	ambarella_board_generic.lcd_backlight.active_level = GPIO_HIGH;
	ambarella_board_generic.lcd_backlight.active_delay = 1;

	ambarella_board_generic.lcd_spi_hw.bus_id = 0;
	ambarella_board_generic.lcd_spi_hw.cs_id = 1;

	ambarella_board_generic.vin_vsync.irq_gpio = GPIO(95);
	ambarella_board_generic.vin_vsync.irq_line = gpio_to_irq(95);
	ambarella_board_generic.vin_vsync.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.vin_vsync.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.vin_vsync.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.flash_charge_ready.irq_gpio = GPIO(13);
	ambarella_board_generic.flash_charge_ready.irq_line = gpio_to_irq(13);
	ambarella_board_generic.flash_charge_ready.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.flash_charge_ready.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.flash_charge_ready.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	ambarella_board_generic.flash_trigger.gpio_id = GPIO(46);
	ambarella_board_generic.flash_trigger.active_level = GPIO_LOW;
	ambarella_board_generic.flash_trigger.active_delay = 1;

	ambarella_board_generic.flash_enable.gpio_id = GPIO(82);
	ambarella_board_generic.flash_enable.active_level = GPIO_LOW;
	ambarella_board_generic.flash_enable.active_delay = 1;

	/* Config ETH*/
	ambarella_eth0_platform_info.mii_reset.gpio_id = GPIO(7);
	ambarella_eth0_platform_info.mii_id = 1;
	ambarella_eth0_platform_info.phy_id = 0x00008201;

	/* Config SD*/
	fio_default_owner = SELECT_FIO_SDIO;
	ambarella_platform_sd_controller0.clk_limit = 25000000;
	ambarella_platform_sd_controller0.slot[0].cd_delay = 100;
	ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
	ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
	ambarella_platform_sd_controller0.slot[1].ext_power.gpio_id = GPIO(54);
	ambarella_platform_sd_controller0.slot[1].ext_power.active_level = GPIO_HIGH;
	ambarella_platform_sd_controller0.slot[1].ext_power.active_delay = 300;
	ambarella_platform_sd_controller0.slot[1].cd_delay = 100;
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio = GPIO(75);
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_line = gpio_to_irq(75);
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
	ambarella_platform_sd_controller0.slot[1].gpio_wp.gpio_id = GPIO(76);

	/* Register devices*/
	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	spi_register_board_info(ambarella_spi_devices,
		ARRAY_SIZE(ambarella_spi_devices));

	i2c_register_board_info(0, &ambarella_ak4642_board_info, 1);

	ambarella_ak4183_board_info.irq =
		ambarella_board_generic.touch_panel_irq.irq_line;
	i2c_register_board_info(0, &ambarella_ak4183_board_info, 1);

	ambarella_chacha_mt4d_board_info.irq =
		ambarella_board_generic.touch_panel_irq.irq_line;
	ambarella_chacha_mt4d_board_info.flags = 0;
	i2c_register_board_info(0, &ambarella_chacha_mt4d_board_info, 1);

	ambarella_tm1510_board_info.irq =
		ambarella_board_generic.touch_panel_irq.irq_line;
	ambarella_tm1510_board_info.flags = 0;
	i2c_register_board_info(0, &ambarella_tm1510_board_info, 1);

	i2c_register_board_info(0, ambarella_board_vin_infos,
		ARRAY_SIZE(ambarella_board_vin_infos));
	i2c_register_board_info(1, &ambarella_board_hdmi_info, 1);

	if (AMBARELLA_BOARD_REV(system_rev) >= 17) {
		i2c_register_board_info(2, &ambarella_isl12022m_board_info, 1);
	} else {
		platform_device_register(&ambarella_rtc0);
	}

	platform_device_register(&coconut_board_input);

	if (AMBARELLA_BOARD_TYPE(system_rev) == AMBARELLA_BOARD_TYPE_VENDOR) {
		switch (AMBARELLA_BOARD_REV(system_rev)) {
			case 11:
#if defined(CONFIG_CODEC_AMBARELLA_WM8737)
				ambarella_init_wm8737(1, 0x1A);	/*i2c-1, 0x1A */
#endif
				break;
			default:
				break;
		}
	}
}

/* ==========================================================================*/
MACHINE_START(COCONUT, "Coconut")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_coconut,
MACHINE_END

