/*
 * arch/arm/plat-ambarella/include/plat/eth.h
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

#ifndef __PLAT_AMBARELLA_ETH_H
#define __PLAT_AMBARELLA_ETH_H

/* ==========================================================================*/
#define AMBETH_MAC_SIZE				(6)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_eth_platform_info {
	u8					mac_addr[AMBETH_MAC_SIZE];
	u32					napi_weight;
	u32					watchdog_timeo;

	u32					phy_id;
	struct ambarella_gpio_irq_info		phy_irq;
	u32					phy_supported;
	u32					mii_id;
	struct ambarella_gpio_io_info		mii_power;
	struct ambarella_gpio_io_info		mii_reset;
	u32					mii_retry_limit;
	u32					mii_retry_tmo;

	u32					default_dma_bus_mode;
	u32					default_dma_opmode;
	u32					default_tx_ring_size;
	u32					default_rx_ring_size;

	int					(*is_enabled)(void);
};
#define AMBA_ETH_PARAM_CALL(id, arg, perm) \
	module_param_cb(eth##id##_napi_weight, &param_ops_uint, &(arg.napi_weight), perm); \
	module_param_cb(eth##id##_watchdog_timeo, &param_ops_uint, &(arg.watchdog_timeo), perm); \
	module_param_cb(eth##id##_phy_id, &param_ops_uint, &(arg.phy_id), perm); \
	AMBA_GPIO_IRQ_MODULE_PARAM_CALL(eth##id##_phy_irq_, arg.phy_irq, perm); \
	module_param_cb(eth##id##_mii_id, &param_ops_uint, &(arg.mii_id), perm); \
	AMBA_GPIO_IO_MODULE_PARAM_CALL(eth##id##_mii_power_, arg.mii_power, perm); \
	AMBA_GPIO_RESET_MODULE_PARAM_CALL(eth##id##_mii_reset_, arg.mii_reset, perm); \
	module_param_cb(eth##id##_mii_retry_limit, &param_ops_uint, &(arg.mii_retry_limit), perm); \
	module_param_cb(eth##id##_mii_retry_tmo, &param_ops_uint, &(arg.mii_retry_tmo), perm);

/* ==========================================================================*/
extern struct platform_device			ambarella_eth0;
extern struct platform_device			ambarella_eth1;
/* ==========================================================================*/
extern int ambarella_init_eth0(const u8 *mac_addr);
extern int ambarella_init_eth1(const u8 *mac_addr);
#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

