/*
 * arch/arm/mach-ambarella/init-generic.c
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

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/leds.h>

#include <plat/ambinput.h>

#include "board-device.h"

#if (AUDIO_CODEC_INSTANCES == 1)
static struct platform_device ambarella_auc_codec0 = {
	.name		= "a2auc-codec",
	.id		= -1,
};
#endif

static struct gpio_led hero3black_led_pins[] = {
        {
                .name                   = "front",
                .default_trigger        = "heartbeat",
                .gpio                   = 45,
                .active_low             = 0,
        },
        {
                .name                   = "top",
                .default_trigger        = "off",
                .gpio                   = 38,
                .active_low             = 0,
        },
        {
                .name                   = "back",
                .default_trigger        = "off",
                .gpio                   = 102,
                .active_low             = 1,
        },
};

static struct gpio_led_platform_data hero3black_led_data = {
        .leds           = hero3black_led_pins,
        .num_leds       = ARRAY_SIZE(hero3black_led_pins),
};

static struct platform_device hero3black_leds = {
        .name   = "leds-gpio",
        .id     = -1,
        .dev    = {
                .platform_data  = &hero3black_led_data,
        }
};

/* ==========================================================================*/
static struct platform_device *ambarella_devices[] __initdata = {
	&ambarella_adc0,
	&ambarella_crypto,
	&ambarella_dummy_codec0,
	&ambarella_eth0,
	&ambarella_eth1,
	&ambarella_fb0,
	&ambarella_fb1,
	&ambarella_i2s0,
	&ambarella_idc0,
	&ambarella_idc1,
	&ambarella_ir0,
	&ambarella_pcm0,
	&ambarella_pwm_platform_device0,
	&ambarella_pwm_platform_device1,
	&ambarella_pwm_platform_device2,
	&ambarella_pwm_platform_device3,
	&ambarella_pwm_platform_device4,
	&ambarella_rtc0,
	&ambarella_sd0,
	&ambarella_spi0,
	&ambarella_spi1,
	&ambarella_spi2,
	&ambarella_uart,
	&ambarella_uart1,
	&ambarella_udc0,
	&ambarella_wdt0,
	&hero3black_leds,
};

/* ==========================================================================*/
static struct spi_board_info ambarella_spi_devices[] = {
	{
		.modalias	= "st7585fb",
		.bus_num	= 2,
		.chip_select	= 7,
	},
};

/* ==========================================================================*/
static struct ambarella_key_table generic_keymap[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_END},
};

static struct ambarella_input_board_info filbert_board_input_info = {
	.pkeymap		= generic_keymap,
	.pinput_dev		= NULL,
	.pdev			= NULL,

	.abx_max_x		= 4095,
	.abx_max_y		= 4095,
	.abx_max_pressure	= 4095,
	.abx_max_width		= 16,
};

struct platform_device filbert_board_input = {
	.name		= "ambarella-input",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &filbert_board_input_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

/* ==========================================================================*/
static void __init ambarella_init_filbert(void)
{
	int					i;
	struct ambarella_spi_platform_info	*spi2_pdata;

	ambarella_init_machine("Filbert");

	for (i = 0; i < 160; i++)
		ambarella_gpio_config(i, GPIO_FUNC_HW);

	/* SPI3, for front panel LCD */
	ambarella_gpio_config(SSI3_CLK,  GPIO_FUNC_HW);
	ambarella_gpio_config(SSI3_MOSI, GPIO_FUNC_HW);
	ambarella_gpio_config(SSI3_MISO, GPIO_FUNC_HW);
	ambarella_gpio_config(SSI3_EN7,  GPIO_FUNC_SW_OUTPUT);

	/* Display reset? */
	ambarella_gpio_config(134,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_set(134, 1);

	ambarella_gpio_config(13,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(34,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(38,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(44,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(45,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(104,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(105,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(113,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(129,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(130,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(132,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(39,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(92,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(94,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(102,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(112,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(131,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(134,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(141,  GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_config(143,  GPIO_FUNC_SW_OUTPUT);
	
	ambarella_gpio_set(13, 0);
	ambarella_gpio_set(34, 0);
	ambarella_gpio_set(38, 0);
	ambarella_gpio_set(44, 0);
	ambarella_gpio_set(45, 0);
	ambarella_gpio_set(104, 0);
	ambarella_gpio_set(105, 0);
	ambarella_gpio_set(113, 0);
	ambarella_gpio_set(129, 0);
	ambarella_gpio_set(130, 0);
	ambarella_gpio_set(132, 0);
	ambarella_gpio_set(39, 1);
	ambarella_gpio_set(92, 1);	
	ambarella_gpio_set(94, 1);
	ambarella_gpio_set(102, 1);
	ambarella_gpio_set(112, 1);
	ambarella_gpio_set(131, 1);
	ambarella_gpio_set(134, 1);
	ambarella_gpio_set(141, 1);
	ambarella_gpio_set(143, 1);

	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	//config board
	ambarella_board_generic.power_detect.irq_gpio = GPIO(135);
	ambarella_board_generic.power_detect.irq_line = gpio_to_irq(135);
	ambarella_board_generic.power_detect.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.power_detect.irq_gpio_val = GPIO_LOW;
	ambarella_board_generic.power_detect.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

	//config LCD
	ambarella_board_generic.lcd_backlight.gpio_id = GPIO(85);
	ambarella_board_generic.lcd_backlight.active_level = GPIO_HIGH;
	ambarella_board_generic.lcd_backlight.active_delay = 1;

	ambarella_board_generic.lcd_spi_hw.bus_id = 2;
	ambarella_board_generic.lcd_spi_hw.cs_id = 4;

	//config audio
	ambarella_board_generic.audio_reset.gpio_id = GPIO(12);
	ambarella_board_generic.audio_reset.active_level = GPIO_LOW;

	/* Config Eth0*/
	ambarella_eth0_platform_info.mii_reset.gpio_id = GPIO(124);
	ambarella_eth0_platform_info.mii_reset.active_level = GPIO_LOW;
	ambarella_eth0_platform_info.mii_reset.active_delay = 20;

	/* Config Eth1*/
	ambarella_eth1_platform_info.mii_reset.gpio_id = GPIO(125);
	ambarella_eth1_platform_info.mii_reset.active_level = GPIO_LOW;
	ambarella_eth1_platform_info.mii_reset.active_delay = 20;

	/* Config Vin*/
	ambarella_board_generic.vin_reset.gpio_id = GPIO(127);
	ambarella_board_generic.vin_reset.active_level = GPIO_LOW;
	ambarella_board_generic.vin_reset.active_delay = 200;

	ambarella_board_generic.vin_vsync.irq_gpio = GPIO(126);
	ambarella_board_generic.vin_vsync.irq_line = gpio_to_irq(126);
	ambarella_board_generic.vin_vsync.irq_type = IRQF_TRIGGER_RISING;
	ambarella_board_generic.vin_vsync.irq_gpio_val = GPIO_HIGH;
	ambarella_board_generic.vin_vsync.irq_gpio_mode = GPIO_FUNC_SW_INPUT;

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

	spi2_pdata = (struct ambarella_spi_platform_info *)ambarella_spi2.dev.platform_data;
	spi2_pdata->cs_num = 7;
	spi2_pdata->cs_pins[7] = -1;
	spi_register_board_info(ambarella_spi_devices,
		ARRAY_SIZE(ambarella_spi_devices));

	ambarella_tm1510_board_info.irq =
		ambarella_board_generic.touch_panel_irq.irq_line;
	ambarella_tm1510_board_info.flags = 0;
	i2c_register_board_info(0, &ambarella_tm1510_board_info, 1);

	i2c_register_board_info(0, ambarella_board_vin_infos,
		ARRAY_SIZE(ambarella_board_vin_infos));

#if (IDC_SUPPORT_PIN_MUXING_FOR_HDMI == 1)
	i2c_register_board_info(0, &ambarella_board_hdmi_info, 1);
#else
	i2c_register_board_info(1, &ambarella_board_hdmi_info, 1);
#endif

	platform_device_register(&filbert_board_input);
}

/* ==========================================================================*/
MACHINE_START(FILBERT, "Filbert")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_filbert,
MACHINE_END

