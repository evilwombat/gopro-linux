/*
 * arch/arm/plat-ambarella/generic/crypto.c
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
#include <linux/dma-mapping.h>

#include <mach/hardware.h>
#include <plat/crypto.h>

/* ==========================================================================*/
#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_HW_CRYPTO
static struct ambarella_platform_crypto_info ambarella_platform_crypto = {
	.reserved = 0,
};

static struct resource ambarella_crypto_resources[] = {
	[0] = {
		.start	= CRYPT_UNIT_BASE,
		.end	= CRYPT_UNIT_BASE + 0x0FFF,
		.name	= "registers",
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= AES_IRQ,
		.end	= AES_IRQ,
		.name	= "aes-irq",
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= DES_IRQ,
		.end	= DES_IRQ,
		.name	= "des-irq",
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device ambarella_crypto = {
	.name		= "ambarella-crypto",
	.id		= -1,
	.resource	= ambarella_crypto_resources,
	.num_resources	= ARRAY_SIZE(ambarella_crypto_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_crypto,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

