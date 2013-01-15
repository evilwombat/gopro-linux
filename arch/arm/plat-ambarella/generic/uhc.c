/*
 * arch/arm/plat-ambarella/generic/uhc.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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
#include <linux/delay.h>

#include <mach/hardware.h>
#include <plat/uhc.h>
#include <hal/hal.h>

static int usb_host_initialized = 0;

static void ambarella_enable_usb_host(struct ambarella_uhc_controller *pdata)
{
	u32 sys_config;
	amb_usb_port_state_t state;

	sys_config = amba_readl(SYS_CONFIG_REG);

	if (sys_config & USB1_IS_HOST)
		pdata->usb1_is_host = 1;

	if (usb_host_initialized == 1)
		return;

	usb_host_initialized = 1;

	/* GPIO8 and GPIO10 are programmed as hardware mode */
	if (sys_config & USB1_IS_HOST)
		amba_setbitsl(GPIO0_AFSEL_REG, 0x00000500);
	/* GPIO7 and GPIO9 are programmed as hardware mode */
	amba_setbitsl(GPIO0_AFSEL_REG, 0x00000280);

	/* Reset usb host controller */
	if (amb_usb_host_soft_reset(HAL_BASE_VP) != AMB_HAL_SUCCESS)
		pr_info("%s: amb_set_usb_port0_state fail!\n", __func__);

	/*
	 * We must enable both of the usb ports first, then we can disable
	 * usb port1 if it is configured as device port.
	 */
	state = amb_get_usb_port1_state(HAL_BASE_VP);
	if (state != AMB_USB_ON && amb_set_usb_port1_state(HAL_BASE_VP, AMB_USB_ON)
			!= AMB_HAL_SUCCESS) {
		pr_info("%s: amb_set_usb_port1_state fail!\n", __func__);
	}

	if (amb_set_usb_port0_state(HAL_BASE_VP, AMB_USB_ON)
			!= AMB_HAL_SUCCESS) {
		pr_info("%s: amb_set_usb_port0_state fail!\n", __func__);
	}

	if (!(sys_config & USB1_IS_HOST) && state != AMB_USB_ON) {
		if (amb_set_usb_port1_state(HAL_BASE_VP, state)
				!= AMB_HAL_SUCCESS) {
			pr_info("%s: amb_set_usb_port1_state fail!\n", __func__);
		}
	}
}

static void ambarella_disable_usb_host(void)
{
	u32 sys_config;

	if (usb_host_initialized == 0)
		return;

	usb_host_initialized = 0;

	sys_config = amba_readl(SYS_CONFIG_REG);

	if (amb_set_usb_port0_state(HAL_BASE_VP, AMB_USB_OFF)
			!= AMB_HAL_SUCCESS) {
		pr_info("%s: amb_set_usb_port0_state fail!\n", __func__);
	}

	if (sys_config & USB1_IS_HOST) {
		if (amb_set_usb_port1_state(HAL_BASE_VP, AMB_USB_OFF)
				!= AMB_HAL_SUCCESS) {
			pr_info("%s: amb_set_usb_port1_state fail!\n", __func__);
		}
	}
}

/* ==========================================================================*/
struct resource ambarella_ehci_resources[] = {
	[0] = {
		.start	= USB_HOST_CTRL_EHCI_BASE,
		.end	= USB_HOST_CTRL_EHCI_BASE + 0xFFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= USB_EHCI_IRQ,
		.end	= USB_EHCI_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};


static struct ambarella_uhc_controller ambarella_platform_ehci_data = {
	.enable_host	= ambarella_enable_usb_host,
	.disable_host	= ambarella_disable_usb_host,
	.usb1_is_host	= 0,
};

struct platform_device ambarella_ehci0 = {
	.name		= "ambarella-ehci",
	.id		= -1,
	.resource	= ambarella_ehci_resources,
	.num_resources	= ARRAY_SIZE(ambarella_ehci_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_ehci_data,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

struct resource ambarella_ohci_resources[] = {
	[0] = {
		.start	= USB_HOST_CTRL_OHCI_BASE,
		.end	= USB_HOST_CTRL_OHCI_BASE + 0xFFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= USB_OHCI_IRQ,
		.end	= USB_OHCI_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct ambarella_uhc_controller ambarella_platform_ohci_data = {
	.enable_host	= ambarella_enable_usb_host,
	.disable_host	= ambarella_disable_usb_host,
};

struct platform_device ambarella_ohci0 = {
	.name		= "ambarella-ohci",
	.id		= -1,
	.resource	= ambarella_ohci_resources,
	.num_resources	= ARRAY_SIZE(ambarella_ohci_resources),
	.dev		= {
		.platform_data		= &ambarella_platform_ohci_data,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

