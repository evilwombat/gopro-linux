/*
 * arch/arm/plat-ambarella/generic/ir.c
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

#include <mach/hardware.h>
#include <plat/ir.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
struct resource ambarella_ir_resources[] = {
	[0] = {
		.start	= IR_BASE,
		.end	= IR_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRIF_IRQ,
		.end	= IRIF_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= IR_IN,
		.end	= IR_IN,
		.flags	= IORESOURCE_IO,
	},
};

struct ambarella_ir_controller ambarella_platform_ir_controller0 = {
	.set_pll		= rct_set_ir_pll,
	.get_pll		= get_ir_freq_hz,

	.protocol		= AMBA_IR_PROTOCOL_NEC,
	.debug			= 0,
};
AMBA_IR_PARAM_CALL(ambarella_platform_ir_controller0, 0644);

struct platform_device ambarella_ir0 = {
	.name		= "ambarella-ir",
	.id		= -1,
	.resource	= ambarella_ir_resources,
	.num_resources	= ARRAY_SIZE(ambarella_ir_resources),
	.dev			= {
		.platform_data		= &ambarella_platform_ir_controller0,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

