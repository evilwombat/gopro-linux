/*
 * arch/arm/plat-ambarella/generic/spi.c
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
#include <plat/spi.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
void ambarella_spi_cs_activate(struct ambarella_spi_cs_config *cs_config)
{
	u8			cs_pin;

	if (cs_config->bus_id >= SPI_MASTER_INSTANCES ||
		cs_config->cs_id >= cs_config->cs_num)
		return;

	cs_pin = cs_config->cs_pins[cs_config->cs_id];
	ambarella_gpio_set(cs_pin, 0);
}

void ambarella_spi_cs_deactivate(struct ambarella_spi_cs_config *cs_config)
{
	u8			cs_pin;

	if (cs_config->bus_id >= SPI_MASTER_INSTANCES ||
		cs_config->cs_id >= cs_config->cs_num)
		return;

	cs_pin = cs_config->cs_pins[cs_config->cs_id];
	ambarella_gpio_set(cs_pin, 1);
}

struct resource ambarella_spi0_resources[] = {
	[0] = {
		.start	= SPI_BASE,
		.end	= SPI_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SSI_IRQ,
		.end	= SSI_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

int ambarella_spi0_cs_pins[] = {
	SSI0_EN0, SSI0_EN1, SSIO_EN2, SSIO_EN3,
	SSI_4_N, SSI_5_N, SSI_6_N, SSI_7_N
};
AMBA_SPI_PARAM_CALL(0, ambarella_spi0_cs_pins, 0644);
struct ambarella_spi_platform_info ambarella_spi0_platform_info = {
	.support_dma		= 0,
#if (SPI_MASTER_INSTANCES == 5 )
	.fifo_entries		= 64,
#else
	.fifo_entries		= 16,
#endif
	.cs_num			= ARRAY_SIZE(ambarella_spi0_cs_pins),
	.cs_pins		= ambarella_spi0_cs_pins,
	.cs_activate		= ambarella_spi_cs_activate,
	.cs_deactivate		= ambarella_spi_cs_deactivate,
	.rct_set_ssi_pll	= rct_set_ssi_pll,
	.get_ssi_freq_hz	= get_ssi_freq_hz,
};

struct platform_device ambarella_spi0 = {
	.name		= "ambarella-spi",
	.id		= 0,
	.resource	= ambarella_spi0_resources,
	.num_resources	= ARRAY_SIZE(ambarella_spi0_resources),
	.dev		= {
		.platform_data		= &ambarella_spi0_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#if (SPI_MASTER_INSTANCES >= 2 )
struct resource ambarella_spi1_resources[] = {
	[0] = {
		.start	= SPI2_BASE,
		.end	= SPI2_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SSI2_IRQ,
		.end	= SSI2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

int ambarella_spi1_cs_pins[] = {SSI2_0EN, -1, -1, -1, -1, -1, -1, -1};
AMBA_SPI_PARAM_CALL(1, ambarella_spi1_cs_pins, 0644);
struct ambarella_spi_platform_info ambarella_spi1_platform_info = {
	.support_dma		= 0,
	.fifo_entries		= 16,
	.cs_num			= ARRAY_SIZE(ambarella_spi1_cs_pins),
	.cs_pins		= ambarella_spi1_cs_pins,
	.cs_activate		= ambarella_spi_cs_activate,
	.cs_deactivate		= ambarella_spi_cs_deactivate,
	.rct_set_ssi_pll	= rct_set_ssi2_pll,
	.get_ssi_freq_hz	= get_ssi2_freq_hz,
};

struct platform_device ambarella_spi1 = {
	.name		= "ambarella-spi",
	.id		= 1,
	.resource	= ambarella_spi1_resources,
	.num_resources	= ARRAY_SIZE(ambarella_spi1_resources),
	.dev		= {
		.platform_data		= &ambarella_spi1_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

#if (SPI_MASTER_INSTANCES >= 3 )
struct resource ambarella_spi2_resources[] = {
	[0] = {
		.start	= SPI3_BASE,
		.end	= SPI3_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SSI3_IRQ,
		.end	= SSI3_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

int ambarella_spi2_cs_pins[] = {
	SSI3_EN0, SSI3_EN1, SSI3_EN2, SSI3_EN3,
	SSI3_EN4, SSI3_EN5, SSI3_EN6, SSI3_EN7,
};
AMBA_SPI_PARAM_CALL(2, ambarella_spi2_cs_pins, 0644);
struct ambarella_spi_platform_info ambarella_spi2_platform_info = {
	.support_dma		= 0,
#if (SPI_MASTER_INSTANCES == 5 )
	.fifo_entries		= 64,
#else
	.fifo_entries		= 16,
#endif
	.cs_num			= ARRAY_SIZE(ambarella_spi2_cs_pins),
	.cs_pins		= ambarella_spi2_cs_pins,
	.cs_activate		= ambarella_spi_cs_activate,
	.cs_deactivate		= ambarella_spi_cs_deactivate,
	.rct_set_ssi_pll	= rct_set_ssi2_pll,
	.get_ssi_freq_hz	= get_ssi2_freq_hz,
};

struct platform_device ambarella_spi2 = {
	.name		= "ambarella-spi",
	.id		= 2,
	.resource	= ambarella_spi2_resources,
	.num_resources	= ARRAY_SIZE(ambarella_spi2_resources),
	.dev		= {
		.platform_data		= &ambarella_spi2_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

#if (SPI_MASTER_INSTANCES >= 5 )
struct resource ambarella_spi3_resources[] = {
	[0] = {
		.start	= SPI4_BASE,
		.end	= SPI4_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SSI4_IRQ,
		.end	= SSI4_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

int ambarella_spi3_cs_pins[] = {SSI4_EN0, -1, -1, -1, -1, -1, -1, -1};
AMBA_SPI_PARAM_CALL(3, ambarella_spi3_cs_pins, 0644);
struct ambarella_spi_platform_info ambarella_spi3_platform_info = {
	.support_dma		= 0,
	.fifo_entries		= 16,
	.cs_num			= ARRAY_SIZE(ambarella_spi3_cs_pins),
	.cs_pins		= ambarella_spi3_cs_pins,
	.cs_activate		= ambarella_spi_cs_activate,
	.cs_deactivate		= ambarella_spi_cs_deactivate,
	.rct_set_ssi_pll	= rct_set_ssi2_pll,
	.get_ssi_freq_hz	= get_ssi2_freq_hz,
};

struct platform_device ambarella_spi3 = {
	.name		= "ambarella-spi",
	.id		= 3,
	.resource	= ambarella_spi3_resources,
	.num_resources	= ARRAY_SIZE(ambarella_spi3_resources),
	.dev		= {
		.platform_data		= &ambarella_spi3_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

struct resource ambarella_spi4_resources[] = {
	[0] = {
		.start	= SSI_DMA_BASE,
		.end	= SSI_DMA_BASE + 0x0FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= SSI_AHB_IRQ,
		.end	= SSI_AHB_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

int ambarella_spi4_cs_pins[] = {SSI_AHB_EN0, -1, -1, -1, -1, -1, -1, -1};
AMBA_SPI_PARAM_CALL(4, ambarella_spi4_cs_pins, 0644);
struct ambarella_spi_platform_info ambarella_spi4_platform_info = {
	.support_dma		= 0,
#if (SPI_MASTER_INSTANCES == 5 )
	.fifo_entries		= 32,
#else
	.fifo_entries		= 16,
#endif
	.cs_num			= ARRAY_SIZE(ambarella_spi4_cs_pins),
	.cs_pins		= ambarella_spi4_cs_pins,
	.cs_activate		= ambarella_spi_cs_activate,
	.cs_deactivate		= ambarella_spi_cs_deactivate,
	.rct_set_ssi_pll	= rct_set_ssi_pll,
	.get_ssi_freq_hz	= get_ssi_freq_hz,
};

struct platform_device ambarella_spi4 = {
	.name		= "ambarella-spi",
	.id		= 4,
	.resource	= ambarella_spi4_resources,
	.num_resources	= ARRAY_SIZE(ambarella_spi4_resources),
	.dev		= {
		.platform_data		= &ambarella_spi4_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};
#endif

