/*
 * arch/arm/plat-ambarella/generic/ahci.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * History:
 *	2010/11/12 - [Cao Rongrong] Created file
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
#include <linux/ahci_platform.h>
#include <linux/irq.h>

/*
 * AHCI: all of the initialization for SATA PHY has been done at Amboot.
 */
static int ambarella_ahci_init(struct device *dev, void __iomem *addr)
{
	int			rval = 0;
	struct irq_desc		*ahci_desc;
	struct irq_chip		*ahci_chip = NULL;

	ahci_desc = irq_to_desc(SATA_IRQ);
	if (ahci_desc)
		ahci_chip = get_irq_desc_chip(ahci_desc);
	if (ahci_chip && ahci_chip->irq_set_type)
		ahci_chip->irq_set_type(&ahci_desc->irq_data,
			IRQ_TYPE_LEVEL_HIGH);
	else
		rval = -ENODEV;

	return rval;
}

static struct resource ambarella_ahci_resource[] = {
	[0] = {
		.start	= AHB_PHYS_BASE + SATA_OFFSET,
		.end	= AHB_PHYS_BASE + SATA_OFFSET + 0xFFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SATA_IRQ,
		.end	= SATA_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct ahci_platform_data ambarella_platform_ahci_controller0 = {
	.init		= ambarella_ahci_init,
	.exit		= NULL,
};

struct platform_device ambarella_ahci0 = {
	.name		= "ahci",
	.id		= -1,
	.resource	= ambarella_ahci_resource,
	.num_resources	= ARRAY_SIZE(ambarella_ahci_resource),
	.dev		= {
		.platform_data		= &ambarella_platform_ahci_controller0,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

