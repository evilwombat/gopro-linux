/*
 * arch/arm/plat-ambarella/generic/uart.c
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
#include <linux/serial_core.h>

#include <mach/memory.h>
#include <mach/hardware.h>
#include <plat/uart.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
static struct uart_port	ambarella_uart_port_resource[] = {
	[0] = {
		.type		= PORT_UART00,
		.iotype		= UPIO_MEM,
		.membase	= (void *)UART0_BASE,
		.mapbase	= (unsigned long)(UART0_BASE - APB_BASE + APB_PHYS_BASE),
		.irq		= UART0_IRQ,
		.uartclk	= 27000000,
		.fifosize	= UART_FIFO_SIZE,
		.line		= 0,
	},
#if (UART_INSTANCES >= 2)
	[1] = {
		.type		= PORT_UART00,
		.iotype		= UPIO_MEM,
		.membase	= (void *)UART1_BASE,
		.mapbase	= (unsigned long)(UART1_BASE - APB_BASE + APB_PHYS_BASE),
		.irq		= UART1_IRQ,
		.uartclk	= 27000000,
		.fifosize	= UART_FIFO_SIZE,
		.line		= 0,
	},
#endif
#if (UART_INSTANCES >= 3)
	[2] = {
		.type		= PORT_UART00,
		.iotype		= UPIO_MEM,
		.membase	= (void *)UART2_BASE,
		.mapbase	= (unsigned long)(UART2_BASE - APB_BASE + APB_PHYS_BASE),
		.irq		= UART2_IRQ,
		.uartclk	= 27000000,
		.fifosize	= UART_FIFO_SIZE,
		.line		= 0,
	},
#endif
#if (UART_INSTANCES >= 4)
	[3] = {
		.type		= PORT_UART00,
		.iotype		= UPIO_MEM,
		.membase	= (void *)UART3_BASE,
		.mapbase	= (unsigned long)(UART3_BASE - APB_BASE + APB_PHYS_BASE),
		.irq		= UART3_IRQ,
		.uartclk	= 27000000,
		.fifosize	= UART_FIFO_SIZE,
		.line		= 0,
	},
#endif
};

#if (CHIP_REV == I1) || (CHIP_REV == A7L)
static void ambarella_uart_stop_tx(unsigned char __iomem *membase)
{
	u32					ier;
	u32					iir;

	ier = amba_readl(membase + UART_IE_OFFSET);
	if ((ier & UART_IE_PTIME) != UART_IE_PTIME)
		amba_writel(membase + UART_IE_OFFSET,
			ier | (UART_IE_PTIME | UART_IE_ETBEI));
	iir = amba_readl(membase + UART_II_OFFSET);
	amba_writel(membase + UART_IE_OFFSET,
		ier & ~(UART_IE_PTIME | UART_IE_ETBEI));
}

static u32 ambarella_uart_read_ms(unsigned char __iomem *membase)
{
	u32					ier;
	u32					ms;

	ier = amba_readl(membase + UART_IE_OFFSET);
	if ((ier & UART_IE_EDSSI) != UART_IE_EDSSI)
		amba_writel(membase + UART_IE_OFFSET, ier | UART_IE_EDSSI);
	ms = amba_readl(membase + UART_MS_OFFSET);
	if ((ier & UART_IE_EDSSI) != UART_IE_EDSSI)
		amba_writel(membase + UART_IE_OFFSET, ier);

	return ms;
}
#else
static void ambarella_uart_stop_tx(unsigned char __iomem *membase)
{
	amba_clrbitsl(membase + UART_IE_OFFSET, UART_IE_ETBEI);
}

#if (UART_INSTANCES >= 2)
static u32 ambarella_uart_read_ms(unsigned char __iomem *membase)
{
	return amba_readl(membase + UART_MS_OFFSET);
}
#endif
#endif

struct ambarella_uart_platform_info ambarella_uart_ports = {
	.total_port_num		= ARRAY_SIZE(ambarella_uart_port_resource),
	.registed_port_num	= 0,
	.amba_port[0]		= {
		.port		= &ambarella_uart_port_resource[0],
		.mcr		= DEFAULT_AMBARELLA_UART_MCR,
		.fcr		= DEFAULT_AMBARELLA_UART_FCR,
		.ier		= DEFAULT_AMBARELLA_UART_IER,
		.stop_tx	= ambarella_uart_stop_tx,
		.set_pll	= rct_set_uart_pll,
		.get_pll	= get_uart_freq_hz,
		.get_ms		= NULL,
	},
#if (UART_INSTANCES >= 2)
	.amba_port[1]		= {
		.port		= &ambarella_uart_port_resource[1],
		.mcr		= DEFAULT_AMBARELLA_UART_MCR,
		.fcr		= DEFAULT_AMBARELLA_UART_FCR,
		.ier		= DEFAULT_AMBARELLA_UART_IER,
		.stop_tx	= ambarella_uart_stop_tx,
		.set_pll	= rct_set_uart_pll,
		.get_pll	= get_uart_freq_hz,
		.get_ms		= ambarella_uart_read_ms,
	},
#endif
#if (UART_INSTANCES >= 3)
	.amba_port[2]		= {
		.port		= &ambarella_uart_port_resource[2],
		.mcr		= DEFAULT_AMBARELLA_UART_MCR,
		.fcr		= DEFAULT_AMBARELLA_UART_FCR,
		.ier		= DEFAULT_AMBARELLA_UART_IER,
		.stop_tx	= ambarella_uart_stop_tx,
		.set_pll	= rct_set_uart_pll,
		.get_pll	= get_uart_freq_hz,
		.get_ms		= ambarella_uart_read_ms,
	},
#endif
#if (UART_INSTANCES >= 4)
	.amba_port[3]		= {
		.port		= &ambarella_uart_port_resource[3],
		.mcr		= DEFAULT_AMBARELLA_UART_MCR,
		.fcr		= DEFAULT_AMBARELLA_UART_FCR,
		.ier		= DEFAULT_AMBARELLA_UART_IER,
		.stop_tx	= ambarella_uart_stop_tx,
		.set_pll	= rct_set_uart_pll,
		.get_pll	= get_uart_freq_hz,
		.get_ms		= ambarella_uart_read_ms,
	},
#endif
};

static const char ambarella_uart_platform_device_name[] = "ambarella-uart";

struct platform_device ambarella_uart = {
	.name		= ambarella_uart_platform_device_name,
	.id		= 0,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &ambarella_uart_ports,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#if (UART_INSTANCES >= 2)
struct platform_device ambarella_uart1 = {
	.name		= ambarella_uart_platform_device_name,
	.id		= 1,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &ambarella_uart_ports,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif
#if (UART_INSTANCES >= 3)
struct platform_device ambarella_uart2 = {
	.name		= ambarella_uart_platform_device_name,
	.id		= 2,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &ambarella_uart_ports,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif
#if (UART_INSTANCES >= 4)
struct platform_device ambarella_uart3 = {
	.name		= ambarella_uart_platform_device_name,
	.id		= 3,
	.resource	= NULL,
	.num_resources	= 0,
	.dev		= {
		.platform_data		= &ambarella_uart_ports,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

