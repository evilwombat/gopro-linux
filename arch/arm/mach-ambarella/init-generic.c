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

#include <plat/ambinput.h>
#include <linux/gpio.h>
#include <linux/leds.h>

#include "board-device.h"

#if (AUDIO_CODEC_INSTANCES == 1)
static struct platform_device ambarella_auc_codec0 = {
	.name		= "a2auc-codec",
	.id		= -1,
};
#endif


static struct gpio_led hero2_led_pins[] = {
        {
                .name                   = "hero2:front",
                .default_trigger        = "heartbeat",
                .gpio                   = 37,
                .active_low             = 1,
        },
        {
                .name                   = "hero2:top",
                .default_trigger        = "off",
                .gpio                   = 46,
                .active_low             = 0,
        },
};

static struct gpio_led_platform_data hero2_led_data = {
        .leds           = hero2_led_pins,
        .num_leds       = ARRAY_SIZE(hero2_led_pins),
};

static struct platform_device hero2_leds = {
        .name   = "leds-gpio",
        .id     = -1,
        .dev    = {
                .platform_data  = &hero2_led_data,
        }
};


/* ==========================================================================*/
static struct platform_device *ambarella_devices[] __initdata = {
	&ambarella_adc0,
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_SATA
	&ambarella_ahci0,
#endif
#if (AUDIO_CODEC_INSTANCES == 1)
	&ambarella_auc_codec0,
#endif
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_HW_CRYPTO
	&ambarella_crypto,
#endif
	&ambarella_dummy_codec0,
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_USB
	&ambarella_ehci0,
#endif
#if (ETH_INSTANCES >= 1)
	&ambarella_eth0,
#endif
#if (ETH_INSTANCES >= 2)
	&ambarella_eth1,
#endif
	&ambarella_fb0,
	&ambarella_fb1,
	&ambarella_i2s0,
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_I2C_MUX
	&ambarella_i2cmux,
#endif
	&ambarella_idc0,
#if (IDC_INSTANCES >= 2)
	&ambarella_idc1,
#endif
	&ambarella_ir0,
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_USB
	&ambarella_ohci0,
#endif
	&ambarella_pcm0,
	&ambarella_pwm_platform_device0,
	&ambarella_pwm_platform_device1,
	&ambarella_pwm_platform_device2,
	&ambarella_pwm_platform_device3,
	&ambarella_pwm_platform_device4,
	&ambarella_rtc0,
	&ambarella_sd0,
#if (SD_INSTANCES >= 2)
	&ambarella_sd1,
#endif
	&ambarella_spi0,
#if (SPI_INSTANCES >= 2)
	&ambarella_spi1,
#endif
#if (SPI_INSTANCES >= 3)
	&ambarella_spi2,
#endif
#if (SPI_INSTANCES >= 4)
	&ambarella_spi3,
#endif
	&ambarella_uart,
#if (UART_INSTANCES >= 2)
	&ambarella_uart1,
#endif
#if (UART_INSTANCES >= 3)
	&ambarella_uart2,
#endif
#if (UART_INSTANCES >= 4)
	&ambarella_uart3,
#endif
	&ambarella_udc0,
	&ambarella_wdt0,
	&ambarella_fsg_device0,
	&ambarella_usb_device0,
	&hero2_leds,
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
#if (SPI_INSTANCES >= 2)
	{
		.modalias	= "spidev",
		.bus_num	= 1,
		.chip_select	= 0,
	}
#endif
};

/* ==========================================================================*/
static struct ambarella_key_table generic_keymap[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},
	{AMBINPUT_VI_SW,	{.vi_sw		= {0,	0,	0}}},

	{AMBINPUT_END},
};

static struct ambarella_input_board_info generic_board_input_info = {
	.pkeymap		= generic_keymap,
	.pinput_dev		= NULL,
	.pdev			= NULL,

	.abx_max_x		= 4095,
	.abx_max_y		= 4095,
	.abx_max_pressure	= 4095,
	.abx_max_width		= 16,
};

struct platform_device generic_board_input = {
	.name		= "ambarella-input",
	.id		= -1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &generic_board_input_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

/* ==========================================================================*/
/* Ti-wlan */
/* TCXO clock values */
#ifdef CONFIG_WL12XX_PLATFORM_DATA
/* Reference clock values */
enum {
	WL12XX_REFCLOCK_19	= 0, /* 19.2 MHz */
	WL12XX_REFCLOCK_26	= 1, /* 26 MHz */
	WL12XX_REFCLOCK_38	= 2, /* 38.4 MHz */
	WL12XX_REFCLOCK_52	= 3, /* 52 MHz */
	WL12XX_REFCLOCK_38_XTAL = 4, /* 38.4 MHz, XTAL */
	WL12XX_REFCLOCK_26_XTAL = 5, /* 26 MHz, XTAL */
};

/* TCXO clock values */
enum {
	WL12XX_TCXOCLOCK_19_2	= 0, /* 19.2MHz */
	WL12XX_TCXOCLOCK_26	= 1, /* 26 MHz */
	WL12XX_TCXOCLOCK_38_4	= 2, /* 38.4MHz */
	WL12XX_TCXOCLOCK_52	= 3, /* 52 MHz */
	WL12XX_TCXOCLOCK_16_368	= 4, /* 16.368 MHz */
	WL12XX_TCXOCLOCK_32_736	= 5, /* 32.736 MHz */
	WL12XX_TCXOCLOCK_16_8	= 6, /* 16.8 MHz */
	WL12XX_TCXOCLOCK_33_6	= 7, /* 33.6 MHz */
};

struct wl12xx_platform_data {
	void (*set_power)(bool enable);
	/* SDIO only: IRQ number if WLAN_IRQ line is used, 0 for SDIO IRQs */
	int irq;
	bool use_eeprom;
	int board_ref_clock;
	int board_tcxo_clock;
	unsigned long platform_quirks;
	bool pwr_in_suspend;

	struct wl1271_if_operations *ops;
};

/* Platform does not support level trigger interrupts */
#define WL12XX_PLATFORM_QUIRK_EDGE_IRQ	BIT(0)

int wl12xx_set_platform_data(const struct wl12xx_platform_data *data);

struct wl12xx_platform_data ambarella_boss_wl12xx_pdata __initdata = {
		.irq = (32 + 24),
		.board_ref_clock = WL12XX_REFCLOCK_26,
		.board_tcxo_clock = WL12XX_TCXOCLOCK_26,
};
#endif

/* ==========================================================================*/
static void __init ambarella_init_generic(void)
{
	int					i;

	ambarella_init_machine("Generic");

	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	spi_register_board_info(ambarella_spi_devices,
		ARRAY_SIZE(ambarella_spi_devices));

	i2c_register_board_info(0, ambarella_board_vin_infos,
		ARRAY_SIZE(ambarella_board_vin_infos));

#if (IDC_SUPPORT_PIN_MUXING_FOR_HDMI == 1)
	i2c_register_board_info(0, &ambarella_board_hdmi_info, 1);
#else
	i2c_register_board_info(1, &ambarella_board_hdmi_info, 1);
#endif

	platform_device_register(&generic_board_input);

#ifdef CONFIG_WL12XX_PLATFORM_DATA
	if (wl12xx_set_platform_data(&ambarella_boss_wl12xx_pdata) < 0) {
		printk("%s: wl12xx_set_platform_data() failed\n", __func__);
	}
#endif

	/* Needed for sdcard */
	ambarella_gpio_config(83, GPIO_FUNC_SW_OUTPUT);
	ambarella_gpio_set(83, 1);

	/* Power button */
	ambarella_gpio_config(48, GPIO_FUNC_SW_INPUT);

	/* Record button */
	ambarella_gpio_config(22, GPIO_FUNC_SW_INPUT);
	ambarella_gpio_config(43, GPIO_FUNC_SW_INPUT);
}

/* ==========================================================================*/
MACHINE_START(AMBARELLA, "Ambarella Media SoC")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_generic,
MACHINE_END

