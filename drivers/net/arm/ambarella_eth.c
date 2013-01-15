/*
 * drivers/net/arm/ambarella_eth.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 * Copyright (C) 2004-2011, Ambarella, Inc.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/time.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/ethtool.h>

#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
#include <mach/boss.h>
#endif
#include <asm/dma.h>

#include <mach/hardware.h>
#include <plat/eth.h>

/* ==========================================================================*/
#define AMBETH_PACKET_MAXFRAME	(1536)
#define AMBETH_RX_COPYBREAK	(1518)
#define AMBETH_RX_RNG_MIN	(8)
#define AMBETH_TX_RNG_MIN	(4)

#define AMBETH_RXDMA_STATUS	(ETH_DMA_STATUS_OVF | ETH_DMA_STATUS_RI | \
				ETH_DMA_STATUS_RU | ETH_DMA_STATUS_RPS | \
				ETH_DMA_STATUS_RWT)
#define AMBETH_RXDMA_INTEN	(ETH_DMA_INTEN_OVE | ETH_DMA_INTEN_RIE | \
				ETH_DMA_INTEN_RUE | ETH_DMA_INTEN_RSE | \
				ETH_DMA_INTEN_RWE)
#define AMBETH_TXDMA_STATUS	(ETH_DMA_STATUS_TI | ETH_DMA_STATUS_TPS | \
				ETH_DMA_STATUS_TU | ETH_DMA_STATUS_TJT | \
				ETH_DMA_STATUS_UNF)
#define AMBETH_TXDMA_INTEN	(ETH_DMA_INTEN_TIE | ETH_DMA_INTEN_TSE | \
				ETH_DMA_INTEN_TUE | ETH_DMA_INTEN_TJE | \
				ETH_DMA_INTEN_UNE)
#define AMBETH_DMA_INTEN	(ETH_DMA_INTEN_NIE | ETH_DMA_INTEN_AIE | \
				ETH_DMA_INTEN_FBE | AMBETH_RXDMA_INTEN | \
				AMBETH_TXDMA_INTEN)

#define AMBETH_TDES0_ATOMIC_CHECK
#undef AMBETH_TDES0_ATOMIC_CHECK_ALL
#define AMBETH_RDES0_ATOMIC_CHECK
#undef AMBETH_RDES0_ATOMIC_CHECK_ALL

/* ==========================================================================*/
struct ambeth_desc {
	u32					status;
	u32					length;
	u32					buffer1;
	u32					buffer2;
} __attribute((packed));

struct ambeth_rng_info {
	struct sk_buff				*skb;
	dma_addr_t				mapping;
};

struct ambeth_tx_rngmng {
	unsigned int				cur_tx;
	unsigned int				dirty_tx;
	struct ambeth_rng_info			*rng_tx;
	struct ambeth_desc			*desc_tx;
};

struct ambeth_rx_rngmng {
	unsigned int				cur_rx;
	unsigned int				dirty_rx;
	struct ambeth_rng_info			*rng_rx;
	struct ambeth_desc			*desc_rx;
};

struct ambeth_info {
	unsigned int				rx_count;
	struct ambeth_rx_rngmng			rx;
	unsigned int				tx_count;
	unsigned int				tx_irq_count;
	struct ambeth_tx_rngmng			tx;
	dma_addr_t				rx_dma_desc;
	dma_addr_t				tx_dma_desc;
	spinlock_t				lock;
	int					oldspeed;
	int					oldduplex;
	int					oldlink;

	struct net_device_stats			stats;
	struct napi_struct			napi;
	struct net_device			*ndev;
	struct mii_bus				new_bus;
	struct phy_device			*phydev;
	uint32_t				msg_enable;

	unsigned char __iomem			*regbase;
	struct ambarella_eth_platform_info	*platform_info;
};

/* ==========================================================================*/
static int msg_level = -1;
module_param (msg_level, int, 0);
MODULE_PARM_DESC (msg_level, "Override default message level");

/* ==========================================================================*/
static inline int ambhw_dma_reset(struct ambeth_info *lp)
{
	int					errorCode = 0;
	u32					counter = 0;

	amba_setbitsl(lp->regbase + ETH_DMA_BUS_MODE_OFFSET,
		ETH_DMA_BUS_MODE_SWR);
	do {
		if (counter++ > 100) {
			errorCode = -EIO;
			break;
		}
		mdelay(1);
	} while (amba_tstbitsl(lp->regbase + ETH_DMA_BUS_MODE_OFFSET,
		ETH_DMA_BUS_MODE_SWR));

	if (errorCode && netif_msg_drv(lp))
		dev_err(&lp->ndev->dev, "DMA Error: Check PHY.\n");

	return errorCode;
}

static inline void ambhw_dma_int_enable(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_INTEN_OFFSET, AMBETH_DMA_INTEN);
}

static inline void ambhw_dma_int_disable(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_INTEN_OFFSET, 0);
}

static inline void ambhw_dma_rx_start(struct ambeth_info *lp)
{
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_SR);
}

static inline void ambhw_dma_rx_stop(struct ambeth_info *lp)
{
	unsigned int				irq_status;
	int					i = 1300;

	amba_clrbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_SR);
	do {
		udelay(1);
		irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((irq_status & ETH_DMA_STATUS_RS_MASK) && --i);
	if ((i <= 0) && netif_msg_drv(lp)) {
		dev_err(&lp->ndev->dev,
			"DMA Error: Stop RX status=0x%x, opmode=0x%x.\n",
			amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET),
			amba_readl(lp->regbase + ETH_DMA_OPMODE_OFFSET));
	}
}

static inline void ambhw_dma_tx_start(struct ambeth_info *lp)
{
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_ST);
}

static inline void ambhw_dma_tx_stop(struct ambeth_info *lp)
{
	unsigned int				irq_status;
	int					i = 1300;

	amba_clrbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_ST);
	do {
		udelay(1);
		irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((irq_status & ETH_DMA_STATUS_TS_MASK) && --i);
	if ((i <= 0) && netif_msg_drv(lp)) {
		dev_err(&lp->ndev->dev,
			"DMA Error: Stop TX status=0x%x, opmode=0x%x.\n",
			amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET),
			amba_readl(lp->regbase + ETH_DMA_OPMODE_OFFSET));
	}
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_FTF);
}

static inline void ambhw_dma_tx_restart(struct ambeth_info *lp, u32 entry)
{
	amba_writel(lp->regbase + ETH_DMA_TX_DESC_LIST_OFFSET,
		(u32)lp->tx_dma_desc + (entry * sizeof(struct ambeth_desc)));
	ambhw_dma_tx_start(lp);
}

static inline void ambhw_dma_tx_poll(struct ambeth_info *lp, u32 entry)
{
	lp->tx.desc_tx[entry].status = ETH_TDES0_OWN;
	amba_writel(lp->regbase + ETH_DMA_TX_POLL_DMD_OFFSET, 0x01);
}

static inline void ambhw_stop_tx_rx(struct ambeth_info *lp)
{
	unsigned int				irq_status;
	int					i = 1300;

	amba_clrbitsl(lp->regbase + ETH_MAC_CFG_OFFSET, ETH_MAC_CFG_RE);
	amba_clrbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET,
		(ETH_DMA_OPMODE_SR | ETH_DMA_OPMODE_ST));
	do {
		udelay(1);
		irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((irq_status & (ETH_DMA_STATUS_TS_MASK |
		ETH_DMA_STATUS_RS_MASK)) && --i);
	if ((i <= 0) && netif_msg_drv(lp)) {
		dev_err(&lp->ndev->dev,
			"DMA Error: Stop TX/RX status=0x%x, opmode=0x%x.\n",
			amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET),
			amba_readl(lp->regbase + ETH_DMA_OPMODE_OFFSET));
	}
	amba_clrbitsl(lp->regbase + ETH_MAC_CFG_OFFSET, ETH_MAC_CFG_TE);
}

static inline void ambhw_set_dma_desc(struct ambeth_info *lp)
{
	amba_writel(lp->regbase + ETH_DMA_RX_DESC_LIST_OFFSET,
		lp->rx_dma_desc);
	amba_writel(lp->regbase + ETH_DMA_TX_DESC_LIST_OFFSET,
		lp->tx_dma_desc);
}

static inline phy_interface_t ambhw_get_interface(struct ambeth_info *lp)
{
	return amba_tstbitsl(lp->regbase + ETH_MAC_CFG_OFFSET,
		ETH_MAC_CFG_PS) ? PHY_INTERFACE_MODE_MII :
		PHY_INTERFACE_MODE_GMII;
}

static inline void ambhw_set_hwaddr(struct ambeth_info *lp, u8 *hwaddr)
{
	u32					val;

	val = (hwaddr[5] << 8) | hwaddr[4];
	amba_writel(lp->regbase + ETH_MAC_MAC0_HI_OFFSET, val);
	udelay(4);
	val = (hwaddr[3] << 24) | (hwaddr[2] << 16) |
		(hwaddr[1] << 8) | hwaddr[0];
	amba_writel(lp->regbase + ETH_MAC_MAC0_LO_OFFSET, val);
}

static inline void ambhw_get_hwaddr(struct ambeth_info *lp, u8 *hwaddr)
{
	u32					hval;
	u32					lval;

	hval = amba_readl(lp->regbase + ETH_MAC_MAC0_HI_OFFSET);
	lval = amba_readl(lp->regbase + ETH_MAC_MAC0_LO_OFFSET);
	hwaddr[5] = ((hval >> 8) & 0xff);
	hwaddr[4] = ((hval >> 0) & 0xff);
	hwaddr[3] = ((lval >> 24) & 0xff);
	hwaddr[2] = ((lval >> 16) & 0xff);
	hwaddr[1] = ((lval >> 8) & 0xff);
	hwaddr[0] = ((lval >> 0) & 0xff);
}

static inline void ambhw_set_link_mode_speed(struct ambeth_info *lp)
{
	u32					val;

	val = amba_readl(lp->regbase + ETH_MAC_CFG_OFFSET);
	switch (lp->oldspeed) {
	case SPEED_1000:
		val &= ~(ETH_MAC_CFG_PS);
		break;
	case SPEED_100:
	case SPEED_10:
	default:
		val |= ETH_MAC_CFG_PS;
		break;
	}
	if (lp->oldduplex) {
		val &= ~(ETH_MAC_CFG_DO);
		val |= ETH_MAC_CFG_DM;
	} else {
		val &= ~(ETH_MAC_CFG_DM);
		val |= ETH_MAC_CFG_DO;
	}
	amba_writel(lp->regbase + ETH_MAC_CFG_OFFSET, val);
}

static inline int ambhw_enable(struct ambeth_info *lp)
{
	int					errorCode = 0;

	ambarella_set_gpio_output(&lp->platform_info->mii_power, 1);
	ambarella_set_gpio_reset(&lp->platform_info->mii_reset);

	errorCode = ambhw_dma_reset(lp);
	if (errorCode)
		goto ambhw_init_exit;

	ambhw_set_hwaddr(lp, lp->ndev->dev_addr);
	amba_writel(lp->regbase + ETH_DMA_BUS_MODE_OFFSET,
		lp->platform_info->default_dma_bus_mode);
	amba_writel(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET, 0);
	amba_writel(lp->regbase + ETH_DMA_OPMODE_OFFSET,
		lp->platform_info->default_dma_opmode);
#if defined(CONFIG_AMBARELLA_ETH_SUPPORT_IPC)
	amba_setbitsl(lp->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_TSF);
#endif
	amba_writel(lp->regbase + ETH_DMA_STATUS_OFFSET,
		amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET));
	amba_writel(lp->regbase + ETH_MAC_CFG_OFFSET,
		(ETH_MAC_CFG_TE | ETH_MAC_CFG_RE));
#if defined(CONFIG_AMBARELLA_ETH_SUPPORT_IPC)
	amba_setbitsl(lp->regbase + ETH_MAC_CFG_OFFSET, ETH_MAC_CFG_IPC);
#endif

ambhw_init_exit:
	return errorCode;
}

static inline void ambhw_disable(struct ambeth_info *lp)
{
	ambhw_stop_tx_rx(lp);
	ambhw_dma_int_disable(lp);
	ambarella_set_gpio_output(&lp->platform_info->mii_power, 0);
	ambarella_set_gpio_output(&lp->platform_info->mii_reset, 1);
}

static inline void ambhw_dump(struct ambeth_info *lp)
{
	u32					i;

	dev_info(&lp->ndev->dev, "RX Info: cur_rx %d, dirty_rx %d.\n",
		lp->rx.cur_rx, lp->rx.dirty_rx);
	dev_info(&lp->ndev->dev, "RX Info: RX descriptor "
		"0x%08x 0x%08x 0x%08x 0x%08x.\n",
		lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].status,
		lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].length,
		lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].buffer1,
		lp->rx.desc_rx[lp->rx.dirty_rx % lp->rx_count].buffer2);
	dev_info(&lp->ndev->dev, "TX Info: cur_tx %d, dirty_tx %d.\n",
		lp->tx.cur_tx, lp->tx.dirty_tx);
	for (i = lp->tx.dirty_tx; i < lp->tx.cur_tx; i++) {
		dev_info(&lp->ndev->dev, "TX Info: TX descriptor[%d] "
			"0x%08x 0x%08x 0x%08x 0x%08x.\n", i,
			lp->tx.desc_tx[i % lp->tx_count].status,
			lp->tx.desc_tx[i % lp->tx_count].length,
			lp->tx.desc_tx[i % lp->tx_count].buffer1,
			lp->tx.desc_tx[i % lp->tx_count].buffer2);
	}
	for (i = 0; i <= 21; i++) {
		dev_info(&lp->ndev->dev, "GMAC[%d]: 0x%08x.\n", i,
		amba_readl(lp->regbase + ETH_MAC_CFG_OFFSET + (i << 2)));
	}
	for (i = 0; i <= 54; i++) {
		dev_info(&lp->ndev->dev, "GDMA[%d]: 0x%08x.\n", i,
		amba_readl(lp->regbase + ETH_DMA_BUS_MODE_OFFSET + (i << 2)));
	}
}

/* ==========================================================================*/
static int ambhw_mdio_read(struct mii_bus *bus,
	int mii_id, int regnum)
{
	struct ambeth_info			*lp;
	int					val;
	int					limit;

	lp = (struct ambeth_info *)bus->priv;

	for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(lp->platform_info->mii_retry_tmo);
	}
	if ((limit <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Preread tmo!\n");
		val = 0xFFFFFFFF;
		goto ambhw_mdio_read_exit;
	}

	val = ETH_MAC_GMII_ADDR_PA(mii_id) | ETH_MAC_GMII_ADDR_GR(regnum);
	val |= ETH_MAC_GMII_ADDR_CR_250_300MHZ | ETH_MAC_GMII_ADDR_GB;
	amba_writel(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET, val);

	for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(lp->platform_info->mii_retry_tmo);
	}
	if ((limit <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Postread tmo!\n");
		val = 0xFFFFFFFF;
		goto ambhw_mdio_read_exit;
	}

	val = amba_readl(lp->regbase + ETH_MAC_GMII_DATA_OFFSET);

ambhw_mdio_read_exit:
	if (netif_msg_hw(lp))
		dev_info(&lp->ndev->dev,
			"MII Read: id[0x%02x], add[0x%02x], val[0x%04x].\n",
			mii_id, regnum, val);

	return val;
}

static int ambhw_mdio_write(struct mii_bus *bus,
	int mii_id, int regnum, u16 value)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;
	int					val;
	int					limit = 0;

	lp = (struct ambeth_info *)bus->priv;

	if (netif_msg_hw(lp))
		dev_info(&lp->ndev->dev,
			"MII Write: id[0x%02x], add[0x%02x], val[0x%04x].\n",
			mii_id, regnum, value);

	for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(lp->platform_info->mii_retry_tmo);
	}
	if ((limit <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Prewrite tmo!\n");
		errorCode = -EIO;
		goto ambhw_mdio_write_exit;
	}

	val = value;
	amba_writel(lp->regbase + ETH_MAC_GMII_DATA_OFFSET, val);
	val = ETH_MAC_GMII_ADDR_PA(mii_id) | ETH_MAC_GMII_ADDR_GR(regnum);
	val |= ETH_MAC_GMII_ADDR_CR_250_300MHZ | ETH_MAC_GMII_ADDR_GW |
		ETH_MAC_GMII_ADDR_GB;
	amba_writel(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET, val);

	for (limit = lp->platform_info->mii_retry_limit; limit > 0; limit--) {
		if (!amba_tstbitsl(lp->regbase + ETH_MAC_GMII_ADDR_OFFSET,
			ETH_MAC_GMII_ADDR_GB))
			break;
		udelay(lp->platform_info->mii_retry_tmo);
	}
	if ((limit <= 0) && netif_msg_hw(lp)) {
		dev_err(&lp->ndev->dev, "MII Error: Postwrite tmo!\n");
		errorCode = -EIO;
		goto ambhw_mdio_write_exit;
	}

ambhw_mdio_write_exit:
	return errorCode;
}

static int ambhw_mdio_reset(struct mii_bus *bus)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;

	lp = (struct ambeth_info *)bus->priv;

	if (netif_msg_hw(lp))
		dev_info(&lp->ndev->dev, "MII Info: Power gpio = %d, "
			"Reset gpio = %d.\n",
			lp->platform_info->mii_power.gpio_id,
			lp->platform_info->mii_reset.gpio_id);

	ambarella_set_gpio_output(&lp->platform_info->mii_power, 0);
	ambarella_set_gpio_output(&lp->platform_info->mii_reset, 1);
	ambarella_set_gpio_output(&lp->platform_info->mii_power, 1);
	ambarella_set_gpio_output(&lp->platform_info->mii_reset, 0);

	return errorCode;
}

/* ==========================================================================*/
static void ambeth_adjust_link(struct net_device *ndev)
{
	struct ambeth_info			*lp;
	unsigned long				flags;
	struct phy_device			*phydev;
	int					need_update = 0;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	phydev = lp->phydev;
	if (phydev->link) {
		if (phydev->duplex != lp->oldduplex) {
			need_update = 1;
			lp->oldduplex = phydev->duplex;
		}
		if (phydev->speed != lp->oldspeed) {
			switch (phydev->speed) {
			case SPEED_1000:
			case SPEED_100:
			case SPEED_10:
				need_update = 1;
				lp->oldspeed = phydev->speed;
				break;
			default:
				if (netif_msg_link(lp))
					dev_warn(&lp->ndev->dev,
						"Unknown Speed(%d).\n",
						phydev->speed);
				break;
			}
		}
		if (lp->oldlink != phydev->link) {
			need_update = 1;
			lp->oldlink = phydev->link;
		}
	} else if (lp->oldlink) {
		need_update = 1;
		lp->oldlink = PHY_DOWN;
		lp->oldspeed = 0;
		lp->oldduplex = -1;
	}

	if (need_update) {
		ambhw_set_link_mode_speed(lp);
		if (netif_msg_link(lp))
			phy_print_status(phydev);
	}
	spin_unlock_irqrestore(&lp->lock, flags);
}

static int ambeth_phy_start(struct ambeth_info *lp)
{
	int					errorCode = 0;
	struct phy_device			*phydev;
	phy_interface_t				interface;
	struct net_device			*ndev;
	int					phy_addr;
	unsigned long				flags;

	spin_lock_irqsave(&lp->lock, flags);
	phydev = lp->phydev;
	spin_unlock_irqrestore(&lp->lock, flags);
	if (phydev)
		goto ambeth_init_phy_exit;

	ndev = lp->ndev;
	lp->oldlink = PHY_DOWN;
	lp->oldspeed = 0;
	lp->oldduplex = -1;

	phy_addr = lp->platform_info->mii_id;
	if ((phy_addr >= 0) && (phy_addr < PHY_MAX_ADDR)) {
		if (lp->new_bus.phy_map[phy_addr]) {
			phydev = lp->new_bus.phy_map[phy_addr];
			if (phydev->phy_id == lp->platform_info->phy_id) {
				goto ambeth_init_phy_default;
			}
		}
		dev_notice(&lp->ndev->dev,
			"Could not find default PHY in %d.\n", phy_addr);
	}
	goto ambeth_init_phy_scan;

ambeth_init_phy_default:
	if (netif_msg_hw(lp))
		dev_info(&lp->ndev->dev, "Find default PHY in %d!\n", phy_addr);
	if (ambarella_is_valid_gpio_irq(&lp->platform_info->phy_irq)) {
		ambarella_gpio_config(lp->platform_info->phy_irq.irq_gpio,
			lp->platform_info->phy_irq.irq_gpio_mode);
		phydev->irq = lp->platform_info->phy_irq.irq_line;
		phydev->irq_flags = lp->platform_info->phy_irq.irq_type;
	}
	goto ambeth_init_phy_connect;

ambeth_init_phy_scan:
	for (phy_addr = 0; phy_addr < PHY_MAX_ADDR; phy_addr++) {
		if (lp->new_bus.phy_map[phy_addr]) {
			phydev = lp->new_bus.phy_map[phy_addr];
			if (phydev->phy_id == lp->platform_info->phy_id)
				goto ambeth_init_phy_connect;
		}
	}
	if (!phydev) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "No PHY device.\n");
		errorCode = -ENODEV;
		goto ambeth_init_phy_exit;
	} else {
		if (netif_msg_drv(lp))
			dev_notice(&lp->ndev->dev,
			"Try PHY[%d] whose id is 0x%08x!\n",
			phydev->addr, phydev->phy_id);
	}

ambeth_init_phy_connect:
	interface = ambhw_get_interface(lp);
	phydev = phy_connect(ndev, dev_name(&phydev->dev),
		&ambeth_adjust_link, 0, interface);
	if (IS_ERR(phydev)) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "Could not attach to PHY!\n");
		errorCode = PTR_ERR(phydev);
		goto ambeth_init_phy_exit;
	}

	phydev->supported &= lp->platform_info->phy_supported;
	phydev->advertising = phydev->supported;

	spin_lock_irqsave(&lp->lock, flags);
	lp->phydev = phydev;
	spin_unlock_irqrestore(&lp->lock, flags);

	errorCode = phy_start_aneg(phydev);

ambeth_init_phy_exit:
	return errorCode;
}

static void ambeth_phy_stop(struct ambeth_info *lp)
{
	struct phy_device			*phydev;
	unsigned long				flags;

	spin_lock_irqsave(&lp->lock, flags);
	phydev = lp->phydev;
	lp->phydev = NULL;
	spin_unlock_irqrestore(&lp->lock, flags);
	if (phydev)
		phy_disconnect(phydev);
}

static inline int ambeth_rx_rngmng_check_skb(struct ambeth_info *lp, u32 entry)
{
	int					errorCode = 0;
	dma_addr_t				mapping;
	struct sk_buff				*skb;

	if (lp->rx.rng_rx[entry].skb == NULL) {
		skb = dev_alloc_skb(AMBETH_PACKET_MAXFRAME);
		if (skb == NULL) {
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"RX Error: dev_alloc_skb.\n");
			errorCode = -ENOMEM;
			goto ambeth_rx_rngmng_skb_exit;
		}
		skb->dev = lp->ndev;
		skb->len = AMBETH_PACKET_MAXFRAME;
		mapping = dma_map_single(&lp->ndev->dev, skb->data,
			skb->len, DMA_FROM_DEVICE);
		lp->rx.rng_rx[entry].skb = skb;
		lp->rx.rng_rx[entry].mapping = mapping;
		lp->rx.desc_rx[entry].buffer1 = mapping;
	}

ambeth_rx_rngmng_skb_exit:
	return errorCode;
}

static inline void ambeth_rx_rngmng_init(struct ambeth_info *lp)
{
	int					i;

	lp->rx.cur_rx = 0;
	lp->rx.dirty_rx = 0;
	for (i = 0; i < lp->rx_count; i++) {
		if (ambeth_rx_rngmng_check_skb(lp, i))
			break;
		lp->rx.desc_rx[i].status = ETH_RDES0_OWN;
		lp->rx.desc_rx[i].length =
			ETH_RDES1_RCH | AMBETH_PACKET_MAXFRAME;
		lp->rx.desc_rx[i].buffer2 = (u32)lp->rx_dma_desc +
			((i + 1) * sizeof(struct ambeth_desc));
	}
	lp->rx.desc_rx[lp->rx_count - 1].buffer2 = (u32)lp->rx_dma_desc;
}

static inline void ambeth_rx_rngmng_refill(struct ambeth_info *lp)
{
	u32					entry;

	while (lp->rx.cur_rx > lp->rx.dirty_rx) {
		entry = lp->rx.dirty_rx % lp->rx_count;
		if (ambeth_rx_rngmng_check_skb(lp, entry))
			break;
		lp->rx.desc_rx[entry].status = ETH_RDES0_OWN;
		lp->rx.dirty_rx++;
	}
}

static inline void ambeth_rx_rngmng_del(struct ambeth_info *lp)
{
	int					i;
	dma_addr_t				mapping;
	struct sk_buff				*skb;

	for (i = 0; i < lp->rx_count; i++) {
		if (lp->rx.rng_rx) {
			skb = lp->rx.rng_rx[i].skb;
			mapping = lp->rx.rng_rx[i].mapping;
			lp->rx.rng_rx[i].skb = NULL;
			lp->rx.rng_rx[i].mapping = 0;
			if (skb) {
				dma_unmap_single(&lp->ndev->dev, mapping,
					skb->len, DMA_FROM_DEVICE);
				dev_kfree_skb(skb);
			}
		}
		if (lp->rx.desc_rx) {
			lp->rx.desc_rx[i].status = 0;
			lp->rx.desc_rx[i].length = 0;
			lp->rx.desc_rx[i].buffer1 = 0xBADF00D0;
			lp->rx.desc_rx[i].buffer2 = 0xBADF00D0;
		}
	}
}

static inline void ambeth_tx_rngmng_init(struct ambeth_info *lp)
{
	int					i;

	lp->tx.cur_tx = 0;
	lp->tx.dirty_tx = 0;
	for (i = 0; i < lp->tx_count; i++) {
		lp->tx.rng_tx[i].mapping = 0 ;
		lp->tx.desc_tx[i].length = (ETH_TDES1_LS | ETH_TDES1_FS |
			ETH_TDES1_TCH);
		lp->tx.desc_tx[i].buffer1 = 0;
		lp->tx.desc_tx[i].buffer2 = (u32)lp->tx_dma_desc +
			((i + 1) * sizeof(struct ambeth_desc));
	}
	lp->tx.desc_tx[lp->tx_count - 1].buffer2 = (u32)lp->tx_dma_desc;
}

static inline void ambeth_tx_rngmng_del(struct ambeth_info *lp)
{
	int					i;
	dma_addr_t				mapping;
	struct sk_buff				*skb;
	unsigned int				dirty_tx;
	u32					entry;
	u32					status;

	for (dirty_tx = lp->tx.dirty_tx; lp->tx.cur_tx > dirty_tx; dirty_tx++) {
		entry = dirty_tx % lp->tx_count;
		if (lp->tx.desc_tx) {
			status = lp->tx.desc_tx[entry].status;
			if (status & ETH_TDES0_OWN)
				lp->stats.tx_dropped++;
		}
	}
	for (i = 0; i < lp->tx_count; i++) {
		if (lp->tx.rng_tx) {
			skb = lp->tx.rng_tx[i].skb;
			mapping = lp->tx.rng_tx[i].mapping;
			lp->tx.rng_tx[i].skb = NULL;
			lp->tx.rng_tx[i].mapping = 0;
			if (skb) {
				dma_unmap_single(&lp->ndev->dev, mapping,
					skb->len, DMA_TO_DEVICE);
				dev_kfree_skb(skb);
			}
		}
		if (lp->tx.desc_tx) {
			lp->tx.desc_tx[i].status = 0;
			lp->tx.desc_tx[i].length = 0;
			lp->tx.desc_tx[i].buffer1 = 0xBADF00D0;
			lp->tx.desc_tx[i].buffer2 = 0xBADF00D0;
		}
	}
}

static inline void ambeth_check_dma_error(struct ambeth_info *lp,
	u32 irq_status)
{
	u32					miss_ov = 0;

	if (unlikely(irq_status & ETH_DMA_STATUS_AIS)) {
		if (irq_status & (ETH_DMA_STATUS_RU | ETH_DMA_STATUS_OVF))
			miss_ov = amba_readl(lp->regbase +
				ETH_DMA_MISS_FRAME_BOCNT_OFFSET);

		if (irq_status & ETH_DMA_STATUS_FBI) {
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Fatal Bus Error 0x%x.\n",
				(irq_status & ETH_DMA_STATUS_EB_MASK));
		}
		if (irq_status & ETH_DMA_STATUS_ETI) {
			if (netif_msg_tx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Early Transmit.\n");
		}
		if (irq_status & ETH_DMA_STATUS_RWT) {
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive Watchdog Timeout.\n");
		}
		if (irq_status & ETH_DMA_STATUS_RPS) {
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive Process Stopped.\n");
		}
		if (irq_status & ETH_DMA_STATUS_RU) {
			if (miss_ov & ETH_DMA_MISS_FRAME_BOCNT_FRAME) {
				lp->stats.rx_dropped +=
					ETH_DMA_MISS_FRAME_BOCNT_HOST(miss_ov);
			}
			ambhw_dma_rx_stop(lp);
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive Buffer Unavailable, %d.\n",
				ETH_DMA_MISS_FRAME_BOCNT_HOST(miss_ov));
		}
		if (irq_status & ETH_DMA_STATUS_UNF) {
			if (netif_msg_tx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Transmit Underflow.\n");
		}
		if (irq_status & ETH_DMA_STATUS_OVF) {
			if (miss_ov & ETH_DMA_MISS_FRAME_BOCNT_FIFO) {
				lp->stats.rx_fifo_errors +=
					ETH_DMA_MISS_FRAME_BOCNT_APP(miss_ov);
			}
			if (netif_msg_rx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Receive FIFO Overflow, %d.\n",
				ETH_DMA_MISS_FRAME_BOCNT_APP(miss_ov));
		}
		if (irq_status & ETH_DMA_STATUS_TJT) {
			lp->stats.tx_errors++;
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Transmit Jabber Timeout.\n");
		}
		if (irq_status & ETH_DMA_STATUS_TPS) {
			if (netif_msg_tx_err(lp))
				dev_err(&lp->ndev->dev,
				"DMA Error: Transmit Process Stopped.\n");
		}
		if (netif_msg_tx_err(lp) || netif_msg_rx_err(lp)) {
			dev_err(&lp->ndev->dev, "DMA Error: Abnormal: 0x%x.\n",
				irq_status);
			ambhw_dump(lp);
		}
	}
}

static inline void ambeth_interrupt_rx(struct ambeth_info *lp, u32 irq_status)
{
	if (irq_status & AMBETH_RXDMA_STATUS) {
		dev_vdbg(&lp->ndev->dev, "RX IRQ[0x%x]!\n", irq_status);
		amba_clrbitsl(lp->regbase + ETH_DMA_INTEN_OFFSET,
			AMBETH_RXDMA_INTEN);
		napi_schedule(&lp->napi);
	}
}

static inline u32 ambeth_check_tdes0_status(struct ambeth_info *lp,
	unsigned int status)
{
	u32					tx_retry = 0;

	if (status & ETH_TDES0_JT) {
		lp->stats.tx_heartbeat_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Jabber Timeout.\n");
	}
	if (status & ETH_TDES0_FF) {
		lp->stats.tx_dropped++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Frame Flushed.\n");
	}
	if (status & ETH_TDES0_PCE) {
		lp->stats.tx_fifo_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"TX Error: Payload Checksum Error.\n");
	}
	if (status & ETH_TDES0_LCA) {
		lp->stats.tx_carrier_errors++;
		dev_err(&lp->ndev->dev, "TX Error: Loss of Carrier.\n");
	}
	if (status & ETH_TDES0_NC) {
		lp->stats.tx_carrier_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: No Carrier.\n");
	}
	if (status & ETH_TDES0_LCO) {
		lp->stats.tx_aborted_errors++;
		lp->stats.collisions++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Late Collision.\n");
	}
	if (status & ETH_TDES0_EC) {
		lp->stats.tx_aborted_errors++;
		lp->stats.collisions += ETH_TDES0_CC(status);
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"TX Error: Excessive Collision %d.\n",
			ETH_TDES0_CC(status));
	}
	if (status & ETH_TDES0_VF) {
		if (netif_msg_drv(lp))
			dev_info(&lp->ndev->dev, "TX Info: VLAN Frame.\n");
	}
	if (status & ETH_TDES0_ED) {
		lp->stats.tx_fifo_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"TX Error: Excessive Deferral.\n");
	}
	if (status & ETH_TDES0_UF) {
		tx_retry = 1;
		if (netif_msg_tx_err(lp)) {
			dev_err(&lp->ndev->dev, "TX Error: Underflow Error.\n");
			ambhw_dump(lp);
		}
	}
	if (status & ETH_TDES0_DB) {
		lp->stats.tx_fifo_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "TX Error: Deferred Bit.\n");
	}

	return tx_retry;
}

static inline void ambeth_interrupt_tx(struct ambeth_info *lp, u32 irq_status)
{
	unsigned int				dirty_tx;
	unsigned int				dirty_to_tx;
	u32					entry;
	u32					status;

	if (irq_status & AMBETH_TXDMA_STATUS) {
		dev_vdbg(&lp->ndev->dev, "cur_tx[%d], dirty_tx[%d], 0x%x.\n",
			lp->tx.cur_tx, lp->tx.dirty_tx, irq_status);
		for (dirty_tx = lp->tx.dirty_tx; dirty_tx < lp->tx.cur_tx;
			dirty_tx++) {
			entry = dirty_tx % lp->tx_count;
			status = lp->tx.desc_tx[entry].status;

			if (status & ETH_TDES0_OWN)
				break;

			if (unlikely(status & ETH_TDES0_ES)) {
#if defined(AMBETH_TDES0_ATOMIC_CHECK)
				if ((status & ETH_TDES0_ES_MASK) ==
					ETH_TDES0_ES) {
					if (netif_msg_probe(lp)) {
						dev_err(&lp->ndev->dev,
						"TX Error: Wrong ES"
						" 0x%08x vs 0x%08x.\n",
						status,
						lp->tx.desc_tx[entry].status);
						ambhw_dump(lp);
					}
					break;
				}
#endif
				if (ambeth_check_tdes0_status(lp, status)) {
					ambhw_dma_tx_stop(lp);
					ambhw_dma_tx_restart(lp, entry);
					ambhw_dma_tx_poll(lp, entry);
					break;
				} else {
					lp->stats.tx_errors++;
				}
			} else {
#if defined(AMBETH_TDES0_ATOMIC_CHECK_ALL)
				udelay(1);
				if (unlikely(status !=
					lp->tx.desc_tx[entry].status)) {
					if (netif_msg_probe(lp)) {
						dev_err(&lp->ndev->dev,
						"TX Error: Wrong status"
						" 0x%08x vs 0x%08x.\n",
						status,
						lp->tx.desc_tx[entry].status);
						ambhw_dump(lp);
					}
				}
#endif
#if defined(CONFIG_AMBARELLA_ETH_SUPPORT_IPC)
				if (unlikely(status & ETH_TDES0_IHE)) {
					if (netif_msg_drv(lp))
						dev_err(&lp->ndev->dev,
						"TX Error: IP Header Error.\n");
				}
#endif
				lp->stats.tx_bytes +=
					lp->tx.rng_tx[entry].skb->len;
				lp->stats.tx_packets++;
			}

			dma_unmap_single(&lp->ndev->dev,
				lp->tx.rng_tx[entry].mapping,
				lp->tx.rng_tx[entry].skb->len,
				DMA_TO_DEVICE);
			dev_kfree_skb_irq(lp->tx.rng_tx[entry].skb);
			lp->tx.rng_tx[entry].skb = NULL;
			lp->tx.rng_tx[entry].mapping = 0;
		}

		dirty_to_tx = lp->tx.cur_tx - dirty_tx;
		if (unlikely(dirty_to_tx > lp->tx_count)) {
			netif_stop_queue(lp->ndev);
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev, "TX Error: TX OV.\n");
			ambhw_dump(lp);
			ambhw_dma_tx_stop(lp);
			ambeth_tx_rngmng_del(lp);
			ambeth_tx_rngmng_init(lp);
			dirty_tx = dirty_to_tx = 0;
		}
		if (likely(dirty_to_tx < (lp->tx_count / 2))) {
			dev_vdbg(&lp->ndev->dev, "TX Info: Now gap %d.\n",
				dirty_to_tx);
			netif_wake_queue(lp->ndev);
		}
		lp->tx.dirty_tx = dirty_tx;
	}
}

static irqreturn_t ambeth_interrupt(int irq, void *dev_id)
{
	struct net_device			*ndev;
	struct ambeth_info			*lp;
	u32					irq_status;
	unsigned long				flags;

	ndev = (struct net_device *)dev_id;
	lp = netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	ambeth_check_dma_error(lp, irq_status);
	ambeth_interrupt_rx(lp, irq_status);
	ambeth_interrupt_tx(lp, irq_status);
	amba_writel(lp->regbase + ETH_DMA_STATUS_OFFSET, irq_status);
	spin_unlock_irqrestore(&lp->lock, flags);

	return IRQ_HANDLED;
}

static int ambeth_start_hw(struct net_device *ndev)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;
	unsigned long				flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	errorCode = ambhw_enable(lp);
	spin_unlock_irqrestore(&lp->lock, flags);
	if (errorCode)
		goto ambeth_start_hw_exit;

	lp->rx.rng_rx = kmalloc((sizeof(struct ambeth_rng_info) *
		lp->rx_count), GFP_KERNEL);
	if (lp->rx.rng_rx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "alloc rng_rx fail.\n");
		errorCode = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	lp->rx.desc_rx = dma_alloc_coherent(&lp->ndev->dev,
		(sizeof(struct ambeth_desc) * lp->rx_count),
		&lp->rx_dma_desc, GFP_KERNEL);
	if (lp->rx.desc_rx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"dma_alloc_coherent desc_rx fail.\n");
		errorCode = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	memset(lp->rx.rng_rx, 0,
		(sizeof(struct ambeth_rng_info) * lp->rx_count));
	memset(lp->rx.desc_rx, 0,
		(sizeof(struct ambeth_desc) * lp->rx_count));
	ambeth_rx_rngmng_init(lp);

	lp->tx.rng_tx = kmalloc((sizeof(struct ambeth_rng_info) *
		lp->tx_count), GFP_KERNEL);
	if (lp->tx.rng_tx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "alloc rng_tx fail.\n");
		errorCode = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	lp->tx.desc_tx = dma_alloc_coherent(&lp->ndev->dev,
		(sizeof(struct ambeth_desc) * lp->tx_count),
		&lp->tx_dma_desc, GFP_KERNEL);
	if (lp->tx.desc_tx == NULL) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"dma_alloc_coherent desc_tx fail.\n");
		errorCode = -ENOMEM;
		goto ambeth_start_hw_exit;
	}
	memset(lp->tx.rng_tx, 0,
		(sizeof(struct ambeth_rng_info) * lp->tx_count));
	memset(lp->tx.desc_tx, 0,
		(sizeof(struct ambeth_desc) * lp->tx_count));
	ambeth_tx_rngmng_init(lp);

	spin_lock_irqsave(&lp->lock, flags);
	ambhw_set_dma_desc(lp);
	ambhw_dma_rx_start(lp);
	ambhw_dma_tx_start(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

ambeth_start_hw_exit:
	return errorCode;
}

static void ambeth_stop_hw(struct net_device *ndev)
{
	struct ambeth_info			*lp;
	unsigned long				flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	ambhw_disable(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	ambeth_tx_rngmng_del(lp);
	if (lp->tx.desc_tx) {
		dma_free_coherent(&lp->ndev->dev,
			(sizeof(struct ambeth_desc) * lp->tx_count),
			lp->tx.desc_tx, lp->tx_dma_desc);
		lp->tx.desc_tx = NULL;
	}
	if (lp->tx.rng_tx) {
		kfree(lp->tx.rng_tx);
		lp->tx.rng_tx = NULL;
	}

	ambeth_rx_rngmng_del(lp);
	if (lp->rx.desc_rx) {
		dma_free_coherent(&lp->ndev->dev,
			(sizeof(struct ambeth_desc) * lp->rx_count),
			lp->rx.desc_rx, lp->rx_dma_desc);
		lp->rx.desc_rx = NULL;
	}
	if (lp->rx.rng_rx) {
		kfree(lp->rx.rng_rx);
		lp->rx.rng_rx = NULL;
	}
}

static int ambeth_open(struct net_device *ndev)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	errorCode = ambeth_start_hw(ndev);
	if (errorCode)
		goto ambeth_open_exit;

#if defined(CONFIG_PLAT_AMBARELLA_BOSS)
        boss_set_irq_owner(ndev->irq, BOSS_IRQ_OWNER_LINUX, 0);
#endif
	errorCode = request_irq(ndev->irq, ambeth_interrupt,
		IRQF_SHARED | IRQF_TRIGGER_HIGH, ndev->name, ndev);
	if (errorCode) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"Request_irq[%d] fail.\n", ndev->irq);
		goto ambeth_open_exit;
	}

	napi_enable(&lp->napi);
	netif_start_queue(ndev);
	ambhw_dma_int_enable(lp);

	netif_carrier_off(ndev);
	errorCode = ambeth_phy_start(lp);

ambeth_open_exit:
	if (errorCode) {
		ambeth_phy_stop(lp);
		ambeth_stop_hw(ndev);
	}

	return errorCode;
}

static int ambeth_stop(struct net_device *ndev)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	netif_stop_queue(ndev);
	napi_disable(&lp->napi);
	flush_scheduled_work();
	free_irq(ndev->irq, ndev);
	ambeth_phy_stop(lp);
	ambeth_stop_hw(ndev);

	return errorCode;
}

static int ambeth_hard_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;
	dma_addr_t				mapping;
	u32					entry;
	unsigned int				dirty_to_tx;
	u32					tx_flag;
	unsigned long				flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	spin_lock_irqsave(&lp->lock, flags);
	entry = lp->tx.cur_tx % lp->tx_count;
	dirty_to_tx = lp->tx.cur_tx - lp->tx.dirty_tx;
	if (dirty_to_tx >= lp->tx_count) {
		netif_stop_queue(ndev);
		errorCode = -ENOMEM;
		spin_unlock_irqrestore(&lp->lock, flags);
		goto ambeth_hard_start_xmit_exit;
	}

	mapping = dma_map_single(&lp->ndev->dev,
		skb->data, skb->len, DMA_TO_DEVICE);
	tx_flag = ETH_TDES1_LS | ETH_TDES1_FS | ETH_TDES1_TCH;
	if (dirty_to_tx >= lp->tx_irq_count) {
		netif_stop_queue(ndev);
		tx_flag |= ETH_TDES1_IC;
	}
#if defined(CONFIG_AMBARELLA_ETH_SUPPORT_IPC)
	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		tx_flag |= ETH_TDES1_CIC_TUI | ETH_TDES1_CIC_HDR;
	}
#endif
	lp->tx.rng_tx[entry].skb = skb;
	lp->tx.rng_tx[entry].mapping = mapping;
	lp->tx.desc_tx[entry].buffer1 = mapping;
	lp->tx.desc_tx[entry].length = ETH_TDES1_TBS1x(skb->len) | tx_flag;
	lp->tx.cur_tx++;
	ambhw_dma_tx_poll(lp, entry);
	spin_unlock_irqrestore(&lp->lock, flags);

	ndev->trans_start = jiffies;
	dev_vdbg(&lp->ndev->dev, "TX Info: cur_tx[%d], dirty_tx[%d], "
		"entry[%d], len[%d], data_len[%d], ip_summed[%d], "
		"csum_start[%d], csum_offset[%d].\n",
		lp->tx.cur_tx, lp->tx.dirty_tx, entry, skb->len, skb->data_len,
		skb->ip_summed, skb->csum_start, skb->csum_offset);

ambeth_hard_start_xmit_exit:
	return errorCode;
}

static void ambeth_timeout(struct net_device *ndev)
{
	struct ambeth_info			*lp;
	unsigned long				flags;
	u32					irq_status;

	lp = (struct ambeth_info *)netdev_priv(ndev);

	dev_info(&lp->ndev->dev, "OOM Info:...\n");
	spin_lock_irqsave(&lp->lock, flags);
	irq_status = amba_readl(lp->regbase + ETH_DMA_STATUS_OFFSET);
	ambeth_interrupt_tx(lp, irq_status | AMBETH_TXDMA_STATUS);
	ambhw_dump(lp);
	spin_unlock_irqrestore(&lp->lock, flags);

	netif_wake_queue(ndev);
}

static struct net_device_stats *ambeth_get_stats(struct net_device *ndev)
{
	struct ambeth_info *lp = netdev_priv(ndev);

	return &lp->stats;
}

static inline void ambeth_check_rdes0_status(struct ambeth_info *lp,
	unsigned int status)
{
	if (status & ETH_RDES0_DE) {
		lp->stats.rx_frame_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Descriptor Error.\n");
	}
	if (status & ETH_RDES0_SAF) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Source Address Filter Fail.\n");
	}
	if (status & ETH_RDES0_LE) {
		lp->stats.rx_length_errors++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "RX Error: Length Error.\n");
	}
	if (status & ETH_RDES0_OE) {
		lp->stats.rx_over_errors++;
		if (netif_msg_rx_err(lp))
			dev_err(&lp->ndev->dev, "RX Error: Overflow Error.\n");
	}
	if (status & ETH_RDES0_VLAN) {
		if (netif_msg_drv(lp))
			dev_info(&lp->ndev->dev, "RX Info: VLAN.\n");
	}
	if (status & ETH_RDES0_IPC) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: IPC Checksum/Giant Frame.\n");
	}
	if (status & ETH_RDES0_LC) {
		lp->stats.collisions++;
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev, "RX Error: Late Collision.\n");
	}
	if (status & ETH_RDES0_FT) {
		if (netif_msg_rx_err(lp))
			dev_info(&lp->ndev->dev,
			"RX Info: Ethernet-type frame.\n");
	}
	if (status & ETH_RDES0_RWT) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Watchdog Timeout.\n");
	}
	if (status & ETH_RDES0_RE) {
		lp->stats.rx_errors++;
		if (netif_msg_rx_err(lp))
			dev_err(&lp->ndev->dev, "RX Error: Receive.\n");
	}
	if (status & ETH_RDES0_DBE) {
		if (amba_tstbitsl(lp->regbase + ETH_MAC_CFG_OFFSET,
			ETH_MAC_CFG_PS)) {
			lp->stats.rx_length_errors++;
			if (netif_msg_drv(lp))
				dev_err(&lp->ndev->dev,
				"RX Error: Dribble Bit.\n");
		}
	}
	if (status & ETH_RDES0_CE) {
		lp->stats.rx_crc_errors++;
		if (netif_msg_rx_err(lp))
			dev_err(&lp->ndev->dev, "RX Error: CRC.\n");
	}
	if (status & ETH_RDES0_RX) {
		if (netif_msg_drv(lp))
			dev_err(&lp->ndev->dev,
			"RX Error: Rx MAC Address/Payload Checksum.\n");
	}
}

static inline void ambeth_napi_rx(struct ambeth_info *lp, u32 status, u32 entry)
{
	short					pkt_len;
	struct sk_buff				*skb;
	dma_addr_t				mapping;

	pkt_len = ETH_RDES0_FL(status) - 4;

	if (unlikely(pkt_len > AMBETH_RX_COPYBREAK)) {
		dev_warn(&lp->ndev->dev, "Bogus packet size %d.\n", pkt_len);
		pkt_len = AMBETH_RX_COPYBREAK;
		lp->stats.rx_length_errors++;
	}

	skb = lp->rx.rng_rx[entry].skb;
	mapping = lp->rx.rng_rx[entry].mapping;
	if (likely(skb && mapping)) {
		dma_unmap_single(&lp->ndev->dev, mapping,
			skb->len, DMA_FROM_DEVICE);
		skb_put(skb, pkt_len);
		skb->protocol = eth_type_trans(skb, lp->ndev);
#if defined(CONFIG_AMBARELLA_ETH_SUPPORT_IPC)
		if ((status & ETH_RDES0_COE_MASK) == ETH_RDES0_COE_NOCHKERROR) {
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		} else {
			skb->ip_summed = CHECKSUM_NONE;
			if (netif_msg_rx_err(lp)) {
				dev_err(&lp->ndev->dev,
				"RX Error: RDES0_COE[0x%x].\n", status);
				ambhw_dump(lp);
			}
		}
#endif
		netif_receive_skb(skb);
		lp->rx.rng_rx[entry].skb = NULL;
		lp->rx.rng_rx[entry].mapping = 0;
		lp->ndev->last_rx = jiffies;
		lp->stats.rx_packets++;
		lp->stats.rx_bytes += pkt_len;
	} else {
		if (netif_msg_drv(lp)) {
			dev_err(&lp->ndev->dev,
			"RX Error: %d skb[%p], map[0x%08X].\n",
			entry, skb, mapping);
			ambhw_dump(lp);
		}
	}
}

int ambeth_napi(struct napi_struct *napi, int budget)
{
	int					rx_budget = budget;
	struct ambeth_info			*lp;
	u32					entry;
	u32					status;
	unsigned long				flags;

	lp = container_of(napi, struct ambeth_info, napi);

	if (unlikely(!netif_carrier_ok(lp->ndev)))
		goto ambeth_poll_complete;

	while (rx_budget > 0) {
		entry = lp->rx.cur_rx % lp->rx_count;
		status = lp->rx.desc_rx[entry].status;
		if (status & ETH_RDES0_OWN)
			break;
#if defined(AMBETH_RDES0_ATOMIC_CHECK)
		if (unlikely((status & (ETH_RDES0_FS | ETH_RDES0_LS)) !=
			(ETH_RDES0_FS | ETH_RDES0_LS))) {
			if (netif_msg_probe(lp)) {
				dev_err(&lp->ndev->dev, "RX Error: Wrong FS/LS"
				" cur_rx[%d] status 0x%08x.\n",
				lp->rx.cur_rx, status);
				ambhw_dump(lp);
			}
			break;
		}
#endif
#if defined(AMBETH_TDES0_ATOMIC_CHECK_ALL)
		udelay(1);
		if (unlikely(status != lp->rx.desc_rx[entry].status)) {
			if (netif_msg_probe(lp)) {
				dev_err(&lp->ndev->dev, "RX Error: Wrong status"
				" 0x%08x vs 0x%08x.\n",
				status, lp->rx.desc_rx[entry].status);
				ambhw_dump(lp);
			}
		}
#endif
		if (likely((status & ETH_RDES0_ES) != ETH_RDES0_ES)) {
			ambeth_napi_rx(lp, status, entry);
		} else {
			ambeth_check_rdes0_status(lp, status);
		}
		rx_budget--;
		lp->rx.cur_rx++;

		if ((lp->rx.cur_rx - lp->rx.dirty_rx) > (lp->rx_count / 4))
			ambeth_rx_rngmng_refill(lp);
	}

ambeth_poll_complete:
	if (rx_budget > 0) {
		ambeth_rx_rngmng_refill(lp);
		spin_lock_irqsave(&lp->lock, flags);
		napi_complete(&lp->napi);
		amba_setbitsl(lp->regbase + ETH_DMA_INTEN_OFFSET,
			AMBETH_RXDMA_INTEN);
		ambhw_dma_rx_start(lp);
		spin_unlock_irqrestore(&lp->lock, flags);
	}

	return (budget - rx_budget);
}

static inline u32 ambhw_hashtable_crc(unsigned char *mac)
{
	unsigned char				tmpbuf[ETH_ALEN];
	int					i;
	u32					crc;

	for (i = 0; i < ETH_ALEN; i++)
		tmpbuf[i] = bitrev8(mac[i]);
	crc = crc32_be(~0, tmpbuf, ETH_ALEN);

	return (crc ^ ~0);
}

static inline void ambhw_hashtable_get(struct net_device *ndev, u32 *hat)
{
	struct netdev_hw_addr			*ha;
	unsigned int				bitnr;
#if 0
	unsigned char test1[] = {0x1F,0x52,0x41,0x9C,0xB6,0xAF};
	unsigned char test2[] = {0xA0,0x0A,0x98,0x00,0x00,0x45};
	dev_info(&ndev->dev,
		"Test1: 0x%08X.\n", ambhw_hashtable_crc(test1));
	dev_info(&ndev->dev,
		"Test2: 0x%08X.\n", ambhw_hashtable_crc(test2));
#endif

	hat[0] = hat[1] = 0;
	netdev_for_each_mc_addr(ha, ndev) {
		if (!(ha->addr[0] & 1))
			continue;
		bitnr = ambhw_hashtable_crc(ha->addr);
		bitnr >>= 26;
		bitnr &= 0x3F;
		hat[bitnr >> 5] |= 1 << (bitnr & 31);
	}
}

static void ambeth_set_multicast_list(struct net_device *ndev)
{
	struct ambeth_info			*lp;
	unsigned int				mac_filter;
	u32					hat[2];
	unsigned long				flags;

	lp = (struct ambeth_info *)netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);

	mac_filter = amba_readl(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET);
	hat[0] = 0;
	hat[1] = 0;

	if (ndev->flags & IFF_PROMISC) {
		mac_filter |= ETH_MAC_FRAME_FILTER_PR;
	} else if (ndev->flags & (~IFF_PROMISC)) {
		mac_filter &= ~ETH_MAC_FRAME_FILTER_PR;
	}

	if (ndev->flags & IFF_ALLMULTI) {
		hat[0] = 0xFFFFFFFF;
		hat[1] = 0xFFFFFFFF;
		mac_filter |= ETH_MAC_FRAME_FILTER_PM;
	} else if (!netdev_mc_empty(ndev)) {
		ambhw_hashtable_get(ndev, hat);
		mac_filter &= ~ETH_MAC_FRAME_FILTER_PM;
		mac_filter |= ETH_MAC_FRAME_FILTER_HMC;
	} else if (ndev->flags & (~IFF_ALLMULTI)) {
		mac_filter &= ~ETH_MAC_FRAME_FILTER_PM;
		mac_filter |= ETH_MAC_FRAME_FILTER_HMC;
	}

	if (netif_msg_hw(lp)) {
		dev_info(&lp->ndev->dev, "MC Info: flags 0x%x.\n", ndev->flags);
		dev_info(&lp->ndev->dev, "MC Info: mc_count 0x%x.\n",
			netdev_mc_count(ndev));
		dev_info(&lp->ndev->dev, "MC Info: mac_filter 0x%x.\n",
			mac_filter);
		dev_info(&lp->ndev->dev, "MC Info: hat[0x%x:0x%x].\n",
			hat[1], hat[0]);
	}

	amba_writel(lp->regbase + ETH_MAC_HASH_HI_OFFSET, hat[1]);
	amba_writel(lp->regbase + ETH_MAC_HASH_LO_OFFSET, hat[0]);
	amba_writel(lp->regbase + ETH_MAC_FRAME_FILTER_OFFSET, mac_filter);

	spin_unlock_irqrestore(&lp->lock, flags);
}

static int ambeth_set_mac_address(struct net_device *ndev, void *addr)
{
	struct ambeth_info			*lp;
	unsigned long				flags;
	struct sockaddr				*saddr;

	lp = (struct ambeth_info *)netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);
	saddr = (struct sockaddr *)(addr);

	if (netif_running(ndev))
		return -EBUSY;

	if (!is_valid_ether_addr(saddr->sa_data))
		return -EADDRNOTAVAIL;

	dev_dbg(&lp->ndev->dev, "MAC address[%pM].\n", saddr->sa_data);

	memcpy(ndev->dev_addr, saddr->sa_data, ndev->addr_len);
	ambhw_set_hwaddr(lp, ndev->dev_addr);
	ambhw_get_hwaddr(lp, ndev->dev_addr);
	memcpy(lp->platform_info->mac_addr, ndev->dev_addr, AMBETH_MAC_SIZE);

	spin_unlock_irqrestore(&lp->lock, flags);

	return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void ambeth_poll_controller(struct net_device *ndev)
{
	ambeth_interrupt(ndev->irq, ndev);
}
#endif

static int ambeth_get_settings(struct net_device *ndev,
	struct ethtool_cmd *ecmd)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;
	unsigned long				flags;

	if (!netif_running(ndev)) {
		errorCode = -EINVAL;
		goto ambeth_get_settings_exit;
	}

	lp = (struct ambeth_info *)netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);
	if (lp->phydev) {
		errorCode = phy_ethtool_gset(lp->phydev, ecmd);
	} else {
		errorCode = -EINVAL;
	}
	spin_unlock_irqrestore(&lp->lock, flags);

ambeth_get_settings_exit:
	return errorCode;
}

static int ambeth_set_settings(struct net_device *ndev,
	struct ethtool_cmd *ecmd)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;
	unsigned long				flags;

	if (!netif_running(ndev)) {
		errorCode = -EINVAL;
		goto ambeth_get_settings_exit;
	}

	lp = (struct ambeth_info *)netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);
	if (lp->phydev) {
		errorCode = phy_ethtool_sset(lp->phydev, ecmd);
	} else {
		errorCode = -EINVAL;
	}
	spin_unlock_irqrestore(&lp->lock, flags);

ambeth_get_settings_exit:
	return errorCode;
}

static int ambeth_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
	int					errorCode = 0;
	struct ambeth_info			*lp;
	unsigned long				flags;

	if (!netif_running(ndev)) {
		errorCode = -EINVAL;
		goto ambeth_get_settings_exit;
	}

	lp = (struct ambeth_info *)netdev_priv(ndev);
	spin_lock_irqsave(&lp->lock, flags);
	if (lp->phydev) {
		errorCode = phy_mii_ioctl(lp->phydev, ifr, cmd);
	} else {
		errorCode = -EINVAL;
	}
	spin_unlock_irqrestore(&lp->lock, flags);

ambeth_get_settings_exit:
	return errorCode;
}

/* ==========================================================================*/
static const struct net_device_ops ambeth_netdev_ops = {
	.ndo_open		= ambeth_open,
	.ndo_stop		= ambeth_stop,
	.ndo_start_xmit		= ambeth_hard_start_xmit,
	.ndo_set_multicast_list	= ambeth_set_multicast_list,
	.ndo_set_mac_address 	= ambeth_set_mac_address,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_do_ioctl		= ambeth_ioctl,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_tx_timeout		= ambeth_timeout,
	.ndo_get_stats		= ambeth_get_stats,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= ambeth_poll_controller,
#endif
};

static const struct ethtool_ops ambeth_ethtool_ops = {
	.get_settings	= ambeth_get_settings,
	.set_settings	= ambeth_set_settings,
	.get_link	= ethtool_op_get_link,
};

static int __devinit ambeth_drv_probe(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct net_device			*ndev;
	struct ambeth_info			*lp;
	struct ambarella_eth_platform_info	*platform_info;
	struct resource				*reg_res;
	struct resource				*irq_res;
	int					i;

	platform_info =
		(struct ambarella_eth_platform_info *)pdev->dev.platform_data;
	if (platform_info == NULL) {
		dev_err(&pdev->dev, "Can't get platform_data!\n");
		errorCode = - EPERM;
		goto ambeth_drv_probe_exit;
	}
	if (platform_info->is_enabled) {
		if (!platform_info->is_enabled()) {
			dev_err(&pdev->dev, "Not enabled, check HW config!\n");
			errorCode = -EPERM;
			goto ambeth_drv_probe_exit;
		}
	}

	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (reg_res == NULL) {
		dev_err(&pdev->dev, "Get reg_res failed!\n");
		errorCode = -ENXIO;
		goto ambeth_drv_probe_exit;
	}

	irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq_res == NULL) {
		dev_err(&pdev->dev, "Get irq_res failed!\n");
		errorCode = -ENXIO;
		goto ambeth_drv_probe_exit;
	}

	ndev = alloc_etherdev(sizeof(struct ambeth_info));
	if (ndev == NULL) {
		dev_err(&pdev->dev, "alloc_etherdev fail.\n");
		errorCode = -ENOMEM;
		goto ambeth_drv_probe_exit;
	}
	SET_NETDEV_DEV(ndev, &pdev->dev);
	ndev->dev.dma_mask = pdev->dev.dma_mask;
	ndev->dev.coherent_dma_mask = pdev->dev.coherent_dma_mask;
	ndev->irq = irq_res->start;
#if defined(CONFIG_AMBARELLA_ETH_SUPPORT_IPC)
	ndev->features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif
	lp = netdev_priv(ndev);
	spin_lock_init(&lp->lock);
	lp->ndev = ndev;
	lp->regbase = (unsigned char __iomem *)reg_res->start;
	lp->platform_info = platform_info;
	lp->rx_count = lp->platform_info->default_rx_ring_size;
	if (lp->rx_count < AMBETH_RX_RNG_MIN)
		lp->rx_count = AMBETH_RX_RNG_MIN;
	lp->tx_count = lp->platform_info->default_tx_ring_size;
	if (lp->tx_count < AMBETH_TX_RNG_MIN)
		lp->tx_count = AMBETH_TX_RNG_MIN;
	lp->tx_irq_count = (lp->tx_count * 2) / 3;
	lp->msg_enable = netif_msg_init(msg_level, NETIF_MSG_DRV);

	if (lp->platform_info->mii_power.gpio_id != -1) {
		errorCode = gpio_request(lp->platform_info->mii_power.gpio_id,
			pdev->name);
		if (errorCode) {
			dev_err(&pdev->dev, "gpio_request %d fail %d.\n",
				lp->platform_info->mii_power.gpio_id,
				errorCode);
			goto ambeth_drv_probe_free_netdev;
		}
	}

	if (lp->platform_info->mii_reset.gpio_id != -1) {
		errorCode = gpio_request(lp->platform_info->mii_reset.gpio_id,
			pdev->name);
		if (errorCode) {
			dev_err(&pdev->dev, "gpio_request %d fail %d.\n",
				lp->platform_info->mii_reset.gpio_id,
				errorCode);
			goto ambeth_drv_probe_free_mii_power;
		}
	}

	if (ambarella_is_valid_gpio_irq(&lp->platform_info->phy_irq)) {
		errorCode = gpio_request(lp->platform_info->phy_irq.irq_gpio,
			pdev->name);
		if (errorCode) {
			dev_err(&pdev->dev,
				"gpio_request %d for IRQ %d fail %d.\n",
				lp->platform_info->phy_irq.irq_gpio,
				lp->platform_info->phy_irq.irq_line,
				errorCode);
			goto ambeth_drv_probe_free_mii_reset;
		}
	}

	lp->new_bus.name = "Ambarella MII Bus",
	lp->new_bus.read = &ambhw_mdio_read,
	lp->new_bus.write = &ambhw_mdio_write,
	lp->new_bus.reset = &ambhw_mdio_reset,
	snprintf(lp->new_bus.id, MII_BUS_ID_SIZE, "%x", pdev->id);
	lp->new_bus.priv = lp;
	lp->new_bus.irq = kmalloc(sizeof(int)*PHY_MAX_ADDR, GFP_KERNEL);
	if (lp->new_bus.irq == NULL) {
		dev_err(&pdev->dev, "alloc new_bus.irq fail.\n");
		errorCode = -ENOMEM;
		goto ambeth_drv_probe_free_mii_gpio_irq;
	}
	for(i = 0; i < PHY_MAX_ADDR; ++i)
		lp->new_bus.irq[i] = PHY_POLL;
	lp->new_bus.parent = &pdev->dev;
	lp->new_bus.state = MDIOBUS_ALLOCATED;
	errorCode = mdiobus_register(&lp->new_bus);
	if (errorCode) {
		dev_err(&pdev->dev, "mdiobus_register fail%d.\n", errorCode);
		goto ambeth_drv_probe_kfree_mdiobus;
	}

	ether_setup(ndev);
	ndev->netdev_ops = &ambeth_netdev_ops;
	ndev->watchdog_timeo = lp->platform_info->watchdog_timeo;
	netif_napi_add(ndev, &lp->napi, ambeth_napi,
		lp->platform_info->napi_weight);
	if (!is_valid_ether_addr(lp->platform_info->mac_addr))
		random_ether_addr(lp->platform_info->mac_addr);
	memcpy(ndev->dev_addr, lp->platform_info->mac_addr, AMBETH_MAC_SIZE);
	ambhw_disable(lp);

	SET_ETHTOOL_OPS(ndev, &ambeth_ethtool_ops);
	errorCode = register_netdev(ndev);
	if (errorCode) {
		dev_err(&pdev->dev, " register_netdev fail%d.\n", errorCode);
		goto ambeth_drv_probe_netif_napi_del;
	}

	platform_set_drvdata(pdev, ndev);
	dev_notice(&pdev->dev, "MAC Address[%pM].\n", ndev->dev_addr);
	goto ambeth_drv_probe_exit;

ambeth_drv_probe_netif_napi_del:
	netif_napi_del(&lp->napi);
	mdiobus_unregister(&lp->new_bus);

ambeth_drv_probe_kfree_mdiobus:
	kfree(lp->new_bus.irq);

ambeth_drv_probe_free_mii_gpio_irq:
	if (ambarella_is_valid_gpio_irq(&lp->platform_info->phy_irq))
		gpio_free(lp->platform_info->phy_irq.irq_gpio);

ambeth_drv_probe_free_mii_reset:
	if (lp->platform_info->mii_reset.gpio_id != -1)
		gpio_free(lp->platform_info->mii_reset.gpio_id);

ambeth_drv_probe_free_mii_power:
	if (lp->platform_info->mii_power.gpio_id != -1)
		gpio_free(lp->platform_info->mii_power.gpio_id);

ambeth_drv_probe_free_netdev:
	free_netdev(ndev);

ambeth_drv_probe_exit:
	return errorCode;
}

static int __devexit ambeth_drv_remove(struct platform_device *pdev)
{
	struct net_device			*ndev;
	struct ambeth_info			*lp;

	ndev = platform_get_drvdata(pdev);
	if (ndev) {
		lp = (struct ambeth_info *)netdev_priv(ndev);
		unregister_netdev(ndev);
		netif_napi_del(&lp->napi);
		if (lp->platform_info->mii_power.gpio_id != -1)
			gpio_free(lp->platform_info->mii_power.gpio_id);
		if (lp->platform_info->mii_reset.gpio_id != -1)
			gpio_free(lp->platform_info->mii_reset.gpio_id);
		if (ambarella_is_valid_gpio_irq(&lp->platform_info->phy_irq))
			gpio_free(lp->platform_info->phy_irq.irq_gpio);
		mdiobus_unregister(&lp->new_bus);
		kfree(lp->new_bus.irq);
		platform_set_drvdata(pdev, NULL);
		free_netdev(ndev);
		dev_notice(&pdev->dev, "Removed.\n");
	}

	return 0;
}

#ifdef CONFIG_PM
static int ambeth_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	int					errorCode = 0;
	struct net_device			*ndev;
	struct ambeth_info			*lp;
	unsigned long				flags;

	ndev = platform_get_drvdata(pdev);
	if (ndev) {
		if (!netif_running(ndev))
			goto ambeth_drv_suspend_exit;

		lp = (struct ambeth_info *)netdev_priv(ndev);

		napi_disable(&lp->napi);
		netif_device_detach(ndev);
		disable_irq(ndev->irq);

		ambeth_phy_stop(lp);

		spin_lock_irqsave(&lp->lock, flags);
		ambhw_disable(lp);
		spin_unlock_irqrestore(&lp->lock, flags);
	}

ambeth_drv_suspend_exit:
	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);
	return errorCode;
}

static int ambeth_drv_resume(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct net_device			*ndev;
	struct ambeth_info			*lp;
	unsigned long				flags;

	ndev = platform_get_drvdata(pdev);
	if (ndev) {
		if (!netif_running(ndev))
			goto ambeth_drv_resume_exit;

		lp = (struct ambeth_info *)netdev_priv(ndev);

		spin_lock_irqsave(&lp->lock, flags);
		errorCode = ambhw_enable(lp);
		ambhw_set_link_mode_speed(lp);
		ambeth_rx_rngmng_init(lp);
		ambeth_tx_rngmng_init(lp);
		ambhw_set_dma_desc(lp);
		ambhw_dma_rx_start(lp);
		ambhw_dma_tx_start(lp);
		ambhw_dma_int_enable(lp);
		spin_unlock_irqrestore(&lp->lock, flags);

		if (errorCode) {
			dev_err(&pdev->dev, "ambhw_enable.\n");
		} else {
			ambeth_set_multicast_list(ndev);
			netif_carrier_off(ndev);
			errorCode = ambeth_phy_start(lp);
			enable_irq(ndev->irq);
			netif_device_attach(ndev);
			napi_enable(&lp->napi);
		}
	}

ambeth_drv_resume_exit:
	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);
	return errorCode;
}
#endif

static struct platform_driver ambeth_driver = {
	.probe		= ambeth_drv_probe,
	.remove		= __devexit_p(ambeth_drv_remove),
#ifdef CONFIG_PM
	.suspend        = ambeth_drv_suspend,
	.resume		= ambeth_drv_resume,
#endif
	.driver = {
		.name	= "ambarella-eth",
		.owner	= THIS_MODULE,
	},
};

static int __init ambeth_init(void)
{
	return platform_driver_register(&ambeth_driver);
}

static void __exit ambeth_exit(void)
{
	platform_driver_unregister(&ambeth_driver);
}

module_init(ambeth_init);
module_exit(ambeth_exit);

MODULE_DESCRIPTION("Ambarella Media Processor Ethernet Driver");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

