/*
 * arch/arm/mach-ambarella/init-a7levk.c
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
#include <linux/mmc/host.h>
#include "board-device.h"
/* ==========================================================================*/
static struct platform_device *ambarella_devices[] __initdata = {
#if (ETH_INSTANCES >= 1)
	&ambarella_eth0,
#endif
	&ambarella_sd0,
#if (SD_INSTANCES >= 2)
	&ambarella_sd1,
#endif
	&ambarella_uart,
};

/* ==========================================================================*/
static void __init ambarella_init_a7l_evk(void)
{
	int					i;

	ambarella_init_machine("A7L_EVK");

	platform_add_devices(ambarella_devices, ARRAY_SIZE(ambarella_devices));
	for (i = 0; i < ARRAY_SIZE(ambarella_devices); i++) {
		device_set_wakeup_capable(&ambarella_devices[i]->dev, 1);
		device_set_wakeup_enable(&ambarella_devices[i]->dev, 0);
	}

	/* Config SD*/
	fio_default_owner = SELECT_FIO_SD2;
	ambarella_platform_sd_controller0.clk_limit = 48000000;
	ambarella_platform_sd_controller0.slot[0].cd_delay = HZ;
	ambarella_platform_sd_controller0.slot[0].use_bounce_buffer = 1;
	ambarella_platform_sd_controller0.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
	ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio = GPIO(67);
	ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_line = gpio_to_irq(67);
	ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
	ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio_val  = GPIO_LOW;
	ambarella_platform_sd_controller0.slot[0].gpio_cd.irq_gpio_mode = GPIO_FUNC_SW_INPUT;
#if (SD_INSTANCES >= 2)
	ambarella_platform_sd_controller1.clk_limit = 48000000;
	ambarella_platform_sd_controller1.slot[0].cd_delay = HZ;
	ambarella_platform_sd_controller1.slot[0].use_bounce_buffer = 1;
	ambarella_platform_sd_controller1.slot[0].max_blk_sz = SD_BLK_SZ_128KB;
#if defined(CONFIG_PLAT_AMBARELLA_A7L_EVK)
		ambarella_platform_sd_controller1.slot[0].fixed_cd = 1;
		ambarella_platform_sd_controller1.slot[0].fixed_wp = 0;
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_gpio = -1;
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_line = -1;
		ambarella_platform_sd_controller1.slot[0].gpio_wp.gpio_id = -1;

		/* WIFI_PWR_EN in A7levk; reserved GPIO in A7lbub*/
		ambarella_platform_sd_controller1.slot[0].ext_power.gpio_id = GPIO(105);
		ambarella_platform_sd_controller1.slot[0].ext_power.active_level = GPIO_HIGH;
		ambarella_platform_sd_controller1.slot[0].ext_power.active_delay = 300;
#else
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_gpio = GPIO(75);
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_line = gpio_to_irq(75);
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_type = IRQ_TYPE_EDGE_BOTH;
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_gpio_val	= GPIO_LOW;
		ambarella_platform_sd_controller1.slot[0].gpio_cd.irq_gpio_mode	= GPIO_FUNC_SW_INPUT;
		ambarella_platform_sd_controller1.slot[0].gpio_wp.gpio_id = GPIO(76);
#endif
#endif /* SD_INSTANCES >= 2 */
}

/* ==========================================================================*/
MACHINE_START(AMBARELLA, "A7L_EVK")
	.boot_params	= CONFIG_AMBARELLA_PARAMS_PHYS,
	.map_io		= ambarella_map_io,
	.reserve	= ambarella_memblock_reserve,
	.init_irq	= ambarella_init_irq,
	.timer		= &ambarella_timer,
	.init_machine	= ambarella_init_a7l_evk,
MACHINE_END
