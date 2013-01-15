/*
 * arch/arm/plat-ambarella/misc/board.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/bootmem.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mmc/host.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/i2c.h>

#include <asm/page.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/board.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
struct ambarella_board_info ambarella_board_generic = {
	.board_chip		= 0,
	.board_type		= 0,
	.board_rev		= 0,
	.eth0_mac		= {0, 0, 0, 0, 0, 0},
	.eth1_mac		= {0, 0, 0, 0, 0, 0},
	.wifi_mac		= {0, 0, 0, 0, 0, 0},
	.ueth_mac		= {0, 0, 0, 0, 0, 0},
	.power_detect	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.power_control	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.debug_led0	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.debug_switch	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.rs485		= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.audio_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.audio_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.audio_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.audio_speaker	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.audio_headphone	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.audio_microphone	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.touch_panel_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.touch_panel_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.touch_panel_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.lcd_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.lcd_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.lcd_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.lcd_backlight	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.lcd_spi_hw = {
		.bus_id		= 0,
		.cs_id		= 0,
	},

	.vin_vsync	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.vin_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.vin_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.vin_trigger	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.vin_strobe	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.vin_hdmi_hpd	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.flash_charge_ready	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.flash_enable	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.flash_trigger	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.avplug_detect	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_HIGH,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},

	.hdmi_extpower	= {
		.gpio_id	= -1,
		.active_level	= GPIO_HIGH,
		.active_delay	= 1,
	},

	.gps_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.gps_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.gps_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.gps_wakeup	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.lens_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.lens_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.lens_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.gyro_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.gyro_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.gyro_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.gyro_hps	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.fm_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.fm_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.fm_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.gsensor_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.gsensor_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.gsensor_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

	.bb_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.bb_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.bb_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.bb_en	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.bb_switch	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},

#if (CHIP_REV == I1)
	.pwm0_config = {
		.period_ns	= 40000,
		.max_duty	= 100,
	},
#else
	.pwm0_config = {
		.period_ns	= 40000,
		.max_duty	= 1000,
	},
#endif
	.pwm1_config = {
		.period_ns	= 10000,
		.max_duty	= 1000,
	},
	.pwm2_config = {
		.period_ns	= 10000,
		.max_duty	= 1000,
	},
	.pwm3_config = {
		.period_ns	= 10000,
		.max_duty	= 1000,
	},
	.pwm4_config = {
		.period_ns	= 10000,
		.max_duty	= 1000,
	},

	.wifi_sd_bus		= -1,
	.wifi_sd_slot		= -1,
	.wifi_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.wifi_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.wifi_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.pmic_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
};
AMBA_BOARD_CALL(ambarella_board_generic, 0644);
EXPORT_SYMBOL(ambarella_board_generic);

