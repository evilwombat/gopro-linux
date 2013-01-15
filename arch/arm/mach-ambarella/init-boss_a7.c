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

#include "board-device.h"
/* ==========================================================================*/
#ifdef CONFIG_AMBARELLA_STREAMMEM
extern struct platform_device amba_streammem;
#endif

#ifdef CONFIG_AMBARELLA_HEAPMEM
extern struct platform_device amba_heap_mem;
#endif

/* ==========================================================================*/
static struct platform_device *ambarella_devices[] __initdata = {
	&ambarella_adc0,
	&ambarella_crypto,
	&ambarella_dummy_codec0,
#if (ETH_INSTANCES >= 1)
	&ambarella_eth0,
#endif
#if (ETH_INSTANCES >= 2)
	&ambarella_eth1,
#endif
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
#ifdef CONFIG_AMBARELLA_STREAMMEM
	&amba_streammem,
#endif
#ifdef CONFIG_AMBARELLA_HEAPMEM
	&amba_heap_mem,
#endif
};

/* ==========================================================================*/
static void __init ambarella_init_filbert(void)
{
	int					i;

	ambarella_init_machine("Filbert");

	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	/* Config Eth0*/
	ambarella_eth0_platform_info.mii_reset.gpio_id = GPIO(124);
	ambarella_eth0_platform_info.mii_reset.active_level = GPIO_LOW;
	ambarella_eth0_platform_info.mii_reset.active_delay = 20;

	/* Config Eth1*/
	ambarella_eth1_platform_info.mii_reset.gpio_id = GPIO(125);
	ambarella_eth1_platform_info.mii_reset.active_level = GPIO_LOW;
	ambarella_eth1_platform_info.mii_reset.active_delay = 20;

	/* Config SD*/
	fio_default_owner = SELECT_FIO_SDIO;
	ambarella_platform_sd_controller0.clk_limit = 48000000;
	ambarella_platform_sd_controller0.slot[0].cd_delay = HZ;
	ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
	ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
	ambarella_platform_sd_controller0.slot[1].cd_delay = HZ;
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_gpio = GPIO(75);
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_line = gpio_to_irq(75);
	ambarella_platform_sd_controller0.slot[1].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
	ambarella_platform_sd_controller0.slot[1].gpio_wp.gpio_id = GPIO(76);
}

/* ==========================================================================*/
MACHINE_START(AMBARELLA, "Filbert_Boss")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_filbert,
MACHINE_END
