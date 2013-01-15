/*
 * arch/arm/plat-ambarella/generic/eth.c
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
#include <linux/ethtool.h>

#include <mach/hardware.h>
#include <plat/eth.h>
#include <hal/hal.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"ambarella_config."

/* ==========================================================================*/
#if (ETH_INSTANCES >= 1)
static struct resource ambarella_eth0_resources[] = {
	[0] = {
		.start	= ETH_BASE,
		.end	= ETH_BASE + 0x1FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= ETH_IRQ,
		.end	= ETH_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

struct ambarella_eth_platform_info ambarella_eth0_platform_info = {
	.mac_addr		= {0, 0, 0, 0, 0, 0},
	.napi_weight		= 32,
	.watchdog_timeo		= (2 * HZ),
	.mii_id			= -1,
	.phy_id			= 0x00008201,
	.phy_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.phy_supported		= SUPPORTED_10baseT_Half |
				SUPPORTED_10baseT_Full |
				SUPPORTED_100baseT_Half |
				SUPPORTED_100baseT_Full |
#if (SUPPORT_GMII == 1)
				SUPPORTED_1000baseT_Half |
				SUPPORTED_1000baseT_Full |
#endif
				SUPPORTED_Autoneg |
				SUPPORTED_MII,
	.mii_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.mii_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.mii_retry_limit	= 200,
	.mii_retry_tmo		= 10,
	.default_dma_bus_mode	= ETH_DMA_BUS_MODE_FB |
				ETH_DMA_BUS_MODE_PBL_32 |
				ETH_DMA_BUS_MODE_DA_RX,
	.default_dma_opmode	= ETH_DMA_OPMODE_TTC_256 |
				ETH_DMA_OPMODE_RTC_64 |
				ETH_DMA_OPMODE_FUF,
	.default_tx_ring_size	= 32,
	.default_rx_ring_size	= 64,
	.is_enabled		= rct_is_eth_enabled,
};
AMBA_ETH_PARAM_CALL(0, ambarella_eth0_platform_info, 0644);

int __init ambarella_init_eth0(const u8 *mac_addr)
{
	int					errCode = 0;

	ambarella_eth0_platform_info.mac_addr[0] = mac_addr[0];
	ambarella_eth0_platform_info.mac_addr[1] = mac_addr[1];
	ambarella_eth0_platform_info.mac_addr[2] = mac_addr[2];
	ambarella_eth0_platform_info.mac_addr[3] = mac_addr[3];
	ambarella_eth0_platform_info.mac_addr[4] = mac_addr[4];
	ambarella_eth0_platform_info.mac_addr[5] = mac_addr[5];

	return errCode;
}

struct platform_device ambarella_eth0 = {
	.name		= "ambarella-eth",
	.id		= 0,
	.resource	= ambarella_eth0_resources,
	.num_resources	= ARRAY_SIZE(ambarella_eth0_resources),
	.dev		= {
		.platform_data		= &ambarella_eth0_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#endif

/* ==========================================================================*/
#if (ETH_INSTANCES >= 2)

static struct resource ambarella_eth1_resources[] = {
	[0] = {
		.start	= ETH2_BASE,
		.end	= ETH2_BASE + 0x1FFF,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= ETH2_IRQ,
		.end	= ETH2_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static int eth1_is_enabled(void)
{
	return 1;
}

struct ambarella_eth_platform_info ambarella_eth1_platform_info = {
	.mac_addr		= {0, 0, 0, 0, 0, 0},
	.napi_weight		= 32,
	.watchdog_timeo		= (2 * HZ),
	.mii_id			= -1,
	.phy_id			= 0x00008201,
	.phy_irq	= {
		.irq_gpio	= -1,
		.irq_line	= -1,
		.irq_type	= -1,
		.irq_gpio_val	= GPIO_LOW,
		.irq_gpio_mode	= GPIO_FUNC_SW_INPUT,
	},
	.phy_supported		= SUPPORTED_10baseT_Half |
				SUPPORTED_10baseT_Full |
				SUPPORTED_100baseT_Half |
				SUPPORTED_100baseT_Full |
				SUPPORTED_Autoneg |
				SUPPORTED_MII,
	.mii_power	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.mii_reset	= {
		.gpio_id	= -1,
		.active_level	= GPIO_LOW,
		.active_delay	= 1,
	},
	.mii_retry_limit	= 200,
	.mii_retry_tmo		= 10,
	.default_dma_bus_mode	= ETH_DMA_BUS_MODE_FB |
				ETH_DMA_BUS_MODE_PBL_32 |
				ETH_DMA_BUS_MODE_DA_RX,
	.default_dma_opmode	= ETH_DMA_OPMODE_TTC_256 |
				ETH_DMA_OPMODE_RTC_64 |
				ETH_DMA_OPMODE_FUF,
	.default_tx_ring_size	= 32,
	.default_rx_ring_size	= 64,
	.is_enabled		= eth1_is_enabled,
};
AMBA_ETH_PARAM_CALL(1, ambarella_eth1_platform_info, 0644);

int __init ambarella_init_eth1(const u8 *mac_addr)
{
	int					errCode = 0;

	ambarella_eth1_platform_info.mac_addr[0] = mac_addr[0];
	ambarella_eth1_platform_info.mac_addr[1] = mac_addr[1];
	ambarella_eth1_platform_info.mac_addr[2] = mac_addr[2];
	ambarella_eth1_platform_info.mac_addr[3] = mac_addr[3];
	ambarella_eth1_platform_info.mac_addr[4] = mac_addr[4];
	ambarella_eth1_platform_info.mac_addr[5] = mac_addr[5];

	return errCode;
}

struct platform_device ambarella_eth1 = {
	.name		= "ambarella-eth",
	.id		= 1,
	.resource	= ambarella_eth1_resources,
	.num_resources	= ARRAY_SIZE(ambarella_eth1_resources),
	.dev		= {
		.platform_data		= &ambarella_eth1_platform_info,
		.dma_mask		= &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	}
};

#endif

