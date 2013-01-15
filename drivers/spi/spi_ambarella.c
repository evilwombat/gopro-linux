/*
 * linux/drivers/spi/spi_ambarella.c
 *
 * History:
 *	2008/03/03 - [Louis Sun]  created file
 *	2009/06/19 - [Zhenwu Xue] ported from 2.6.22.10

 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <asm/io.h>
#include <mach/io.h>
#include <mach/dma.h>
#include <plat/spi.h>
#include <plat/ambcache.h>

/*============================Global Variables================================*/

struct ambarella_spi {
	u32					regbase;
	struct ambarella_spi_platform_info	*pinfo;

	int					irq;
	struct tasklet_struct			tasklet;

	spinlock_t				lock;
	struct list_head			queue;
	u32					idle;
	u32					shutdown;

	struct spi_device			*c_dev;
	struct spi_message			*c_msg;
	struct spi_transfer			*c_xfer;

	u8					rw_mode, bpw, chip_select;
	u32					ridx, widx, len;
};

struct ambarella_spi_private {
	struct spi_device			*spi;
	struct mutex				mtx;
	spinlock_t				lock;
};

static struct {
	int					cs_num;
	struct ambarella_spi_private		*data;
} ambarella_spi_private_devices[SPI_MASTER_INSTANCES];

const static u8 ambarella_spi_reverse_table[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static void ambarella_spi_handle_message(struct ambarella_spi *);
static void ambarella_spi_prepare_message(struct ambarella_spi *);
static void ambarella_spi_prepare_transfer(struct ambarella_spi *);
static void ambarella_spi_finish_transfer(struct ambarella_spi *);
static void ambarella_spi_finish_message(struct ambarella_spi *);
static void ambarella_spi_start_transfer(struct ambarella_spi *);
#if (SPI_AHB_INSTANCES > 0)
static void ambarella_spi_dma_complete(void *, u32);
#endif


/*============================SPI Bus Driver==================================*/

static u8 ambarella_spi_bit_reverse_8(u8 x)
{
	return ambarella_spi_reverse_table[x];
}

static u16 ambarella_spi_bit_reverse_16(u16 x)
{
	u16 y;

	y = ambarella_spi_reverse_table[x >> 8];
	y |= (ambarella_spi_reverse_table[x & 0xff] << 8);

	return y;
}

static int ambarella_spi_setup(struct spi_device *spi)
{
	return 0;
}

static int ambarella_spi_stop(struct ambarella_spi *priv)
{
	amba_readl(priv->regbase + SPI_ICR_OFFSET);
	amba_readl(priv->regbase + SPI_ISR_OFFSET);
	amba_writel(priv->regbase + SPI_SER_OFFSET, 0);
	amba_writel(priv->regbase + SPI_SSIENR_OFFSET, 0);

	return 0;
}

static void ambarella_spi_start_transfer(struct ambarella_spi *priv)
{
	void 				*wbuf, *rbuf;
	u32				widx, ridx, len;
	u32				xfer_len;
	u8				cs_id, lsb;
	u16				i, tmp;

	wbuf	= (void *)priv->c_xfer->tx_buf;
	rbuf	= (void *)priv->c_xfer->rx_buf;
	len	= priv->len;
	cs_id	= priv->c_dev->chip_select;
	lsb	= priv->c_dev->mode & SPI_LSB_FIRST;
	widx	= priv->widx;
	ridx	= priv->ridx;

#if (SPI_AHB_INSTANCES > 0)
	if (!priv->pinfo->support_dma) {
#endif
		/* Feed data into FIFO */
		switch (priv->rw_mode) {
		case SPI_WRITE_ONLY:
			xfer_len = len - widx;
			if (xfer_len > priv->pinfo->fifo_entries)
				xfer_len = priv->pinfo->fifo_entries;

			amba_writel(priv->regbase + SPI_SER_OFFSET, 0);
			if (priv->bpw <= 8) {
				for(i = 0; i < xfer_len; i++) {
					tmp = ((u8 *)wbuf)[widx++];
					if (lsb) {
						tmp = ambarella_spi_bit_reverse_8(tmp);
						tmp = tmp >> (8 - priv->bpw);
					}
					amba_writel(priv->regbase + SPI_DR_OFFSET, tmp);
				}
			} else{
				for(i = 0; i < xfer_len; i++) {
					tmp = ((u16 *)wbuf)[widx++];
					if (lsb) {
						tmp = ambarella_spi_bit_reverse_16(tmp);
						tmp = tmp >> (16 - priv->bpw);
					}
					amba_writel(priv->regbase + SPI_DR_OFFSET, tmp);
				}
			}
			amba_writel(priv->regbase + SPI_SER_OFFSET, 1 << cs_id);

			break;

		case SPI_WRITE_READ:
			xfer_len = len - widx;
			if (xfer_len > priv->pinfo->fifo_entries)
				xfer_len = priv->pinfo->fifo_entries;

			amba_writel(priv->regbase + SPI_SER_OFFSET, 0);
			if (priv->bpw <= 8) {
				for(i = 0; i < xfer_len; i++) {
					tmp = ((u8 *)wbuf)[widx++];
					if (lsb) {
						tmp = ambarella_spi_bit_reverse_8(tmp);
						tmp = tmp >> (8 - priv->bpw);
					}
					amba_writel(priv->regbase + SPI_DR_OFFSET, tmp);
				}
			} else{
				for(i = 0; i < xfer_len; i++) {
					tmp = ((u16 *)wbuf)[widx++];
					if (lsb) {
						tmp = ambarella_spi_bit_reverse_16(tmp);
						tmp = tmp >> (16 - priv->bpw);
					}
					amba_writel(priv->regbase + SPI_DR_OFFSET, tmp);
				}
			}
			amba_writel(priv->regbase + SPI_SER_OFFSET, 1 << cs_id);

			break;

		case SPI_READ_ONLY:
			xfer_len = len - ridx;
			if (xfer_len > priv->pinfo->fifo_entries)
				xfer_len = priv->pinfo->fifo_entries;

			amba_writel(priv->regbase + SPI_SER_OFFSET, 0);
			for(i = 0; i < xfer_len; i++)
				amba_writel(priv->regbase + SPI_DR_OFFSET, SPI_DUMMY_DATA);
			amba_writel(priv->regbase + SPI_SER_OFFSET, 1 << cs_id);

			break;

		default:
			break;
		}

		priv->widx = widx;
		enable_irq(priv->irq);
#if (SPI_AHB_INSTANCES > 0)
	} else {
		ambarella_dma_req_t	tx_dma, rx_dma;
		u32			dma_switch_reg, val;

		dma_switch_reg = AHB_SCRATCHPAD_REG(0xc);
		memset(&tx_dma, 0, sizeof(tx_dma));
		memset(&rx_dma, 0, sizeof(rx_dma));

		switch (priv->rw_mode) {
		case SPI_WRITE_ONLY:
			ambarella_dma_enable_irq(SPDIF_AHB_SSI_DMA_CHAN, ambarella_spi_dma_complete);
			tx_dma.src		= ambarella_virt_to_phys((u32)wbuf);
			tx_dma.dst		= ambarella_virt_to_phys(priv->regbase + SPI_DR_OFFSET);
			tx_dma.attr		= DMA_CHANX_CTR_RM | DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_8B;
			if (priv->bpw <= 8) {
				tx_dma.xfr_count	= len;
				tx_dma.attr		|= DMA_CHANX_CTR_TS_1B;
			} else {
				tx_dma.xfr_count	= len << 1;
				tx_dma.attr |= DMA_CHANX_CTR_TS_2B;
			}
			ambcache_clean_range(wbuf, len);
			ambarella_dma_xfr(&tx_dma, MS_AHB_SSI_TX_DMA_CHAN);
			rx_dma.src		= ambarella_virt_to_phys(priv->regbase + SPI_DR_OFFSET);
			rx_dma.dst		= ambarella_virt_to_phys((u32)wbuf);
			rx_dma.attr		= DMA_CHANX_CTR_WM | DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_8B;
			if (priv->bpw <= 8) {
				rx_dma.xfr_count	= len;
				rx_dma.attr		|= DMA_CHANX_CTR_TS_1B;
			} else {
				rx_dma.xfr_count	= len << 1;
				rx_dma.attr |= DMA_CHANX_CTR_TS_2B;
			}
			ambarella_dma_xfr(&rx_dma, SPDIF_AHB_SSI_DMA_CHAN);
			val = amba_readl(dma_switch_reg);
			amba_writel(dma_switch_reg, val | 0x1);
			amba_writel(priv->regbase + SPI_DMAC_OFFSET, 3);

			break;

		case SPI_WRITE_READ:
			ambarella_dma_enable_irq(SPDIF_AHB_SSI_DMA_CHAN, ambarella_spi_dma_complete);
			tx_dma.src		= ambarella_virt_to_phys((u32)wbuf);
			tx_dma.dst		= ambarella_virt_to_phys(priv->regbase + SPI_DR_OFFSET);
			tx_dma.attr		= DMA_CHANX_CTR_RM | DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_8B;
			if (priv->bpw <= 8) {
				tx_dma.xfr_count	= len;
				tx_dma.attr		|= DMA_CHANX_CTR_TS_1B;
			} else {
				tx_dma.xfr_count	= len << 1;
				tx_dma.attr |= DMA_CHANX_CTR_TS_2B;
			}
			ambcache_clean_range(wbuf, len);
			ambarella_dma_xfr(&tx_dma, MS_AHB_SSI_TX_DMA_CHAN);
			rx_dma.src		= ambarella_virt_to_phys(priv->regbase + SPI_DR_OFFSET);
			rx_dma.dst		= ambarella_virt_to_phys((u32)rbuf);
			rx_dma.attr		= DMA_CHANX_CTR_WM | DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_8B;
			if (priv->bpw <= 8) {
				rx_dma.xfr_count	= len;
				rx_dma.attr		|= DMA_CHANX_CTR_TS_1B;
			} else {
				rx_dma.xfr_count	= len << 1;
				rx_dma.attr |= DMA_CHANX_CTR_TS_2B;
			}
			ambarella_dma_xfr(&rx_dma, SPDIF_AHB_SSI_DMA_CHAN);
			val = amba_readl(dma_switch_reg);
			amba_writel(dma_switch_reg, val | 0x1);
			amba_writel(priv->regbase + SPI_DMAC_OFFSET, 3);

			break;

		case SPI_READ_ONLY:
			ambarella_dma_enable_irq(SPDIF_AHB_SSI_DMA_CHAN, ambarella_spi_dma_complete);
			tx_dma.src		= ambarella_virt_to_phys((u32)rbuf);
			tx_dma.dst		= ambarella_virt_to_phys(priv->regbase + SPI_DR_OFFSET);
			tx_dma.attr		= DMA_CHANX_CTR_RM | DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_8B;
			if (priv->bpw <= 8) {
				tx_dma.xfr_count	= len;
				tx_dma.attr		|= DMA_CHANX_CTR_TS_1B;
			} else {
				tx_dma.xfr_count	= len << 1;
				tx_dma.attr |= DMA_CHANX_CTR_TS_2B;
			}
			ambarella_dma_xfr(&tx_dma, MS_AHB_SSI_TX_DMA_CHAN);
			rx_dma.src		= ambarella_virt_to_phys(priv->regbase + SPI_DR_OFFSET);
			rx_dma.dst		= ambarella_virt_to_phys((u32)rbuf);
			rx_dma.attr		= DMA_CHANX_CTR_WM | DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_8B;
			if (priv->bpw <= 8) {
				rx_dma.xfr_count	= len;
				rx_dma.attr		|= DMA_CHANX_CTR_TS_1B;
			} else {
				rx_dma.xfr_count	= len << 1;
				rx_dma.attr |= DMA_CHANX_CTR_TS_2B;
			}
			ambarella_dma_xfr(&rx_dma, SPDIF_AHB_SSI_DMA_CHAN);
			val = amba_readl(dma_switch_reg);
			amba_writel(dma_switch_reg, val | 0x1);
			amba_writel(priv->regbase + SPI_DMAC_OFFSET, 3);

			break;

		default:
			break;
		}

		amba_writel(priv->regbase + SPI_SER_OFFSET, 1 << cs_id);
	}
#endif

	return;
}

static void ambarella_spi_tasklet(unsigned long data)
{
	struct ambarella_spi		*priv	= (struct ambarella_spi *)data;
	void 				*rbuf;
	u32				widx, ridx, len;
	u32				rxflr, xfer_len;
	u32				status;
	u8				lsb;
	u16				i, tmp;
	u32				finish_transfer;

	/* Wait until SPI idle */
	status = amba_readl(priv->regbase + SPI_SR_OFFSET);
	if (status & 0x1) {
		/* Transfer is still in progress */
		for (i = 0; i < MAX_QUERY_TIMES; i++) {
			status = amba_readl(priv->regbase + SPI_SR_OFFSET);
			if (!(status & 0x1))
				break;
		}
		if (status & 0x1) {
			tasklet_schedule(&priv->tasklet);
			return;
		}
	}

	rbuf	= (void *)priv->c_xfer->rx_buf;
	len	= priv->len;
	lsb	= priv->c_dev->mode & SPI_LSB_FIRST;
	widx	= priv->widx;
	ridx	= priv->ridx;

	/* Fetch data from FIFO */
	switch (priv->rw_mode) {
	case SPI_READ_ONLY:
	case SPI_WRITE_READ:
		xfer_len	= len - ridx;
		rxflr		= amba_readl(priv->regbase + SPI_RXFLR_OFFSET);
		if (xfer_len > rxflr)
			xfer_len = rxflr;

		if (priv->bpw <= 8) {
			for(i = 0; i < xfer_len; i++) {
				tmp	= amba_readl(priv->regbase + SPI_DR_OFFSET);
				if (lsb) {
					tmp	= ambarella_spi_bit_reverse_8(tmp);
					tmp	= tmp >> (8 - priv->bpw);
				}
				((u8 *)rbuf)[ridx++]	= tmp & 0xff;
			}
		} else {
			for(i = 0; i < xfer_len; i++){
				tmp	= amba_readl(priv->regbase + SPI_DR_OFFSET);
				if (lsb) {
					tmp	= ambarella_spi_bit_reverse_16(tmp);
					tmp	= tmp >> (16 - priv->bpw);
				}
				((u16 *)rbuf)[ridx++]	= tmp;
			}
		}

		priv->ridx	= ridx;
		break;

	default:
		break;
	}

	/* Check whether the current transfer ends */
	finish_transfer = 0;
	switch (priv->rw_mode) {
	case SPI_WRITE_ONLY:
		if (widx == len) {
			finish_transfer = 1;
		}
		break;

	case SPI_READ_ONLY:
		if (ridx == len) {
			finish_transfer = 1;
		}
		break;

	case SPI_WRITE_READ:
		if (ridx == len && widx == len) {
			finish_transfer = 1;
		}
		break;

	default:
		break;
	}

	/* End transfer or continue filling FIFO */
	if (finish_transfer) {
		ambarella_spi_finish_transfer(priv);
		enable_irq(priv->irq);
	} else {
		ambarella_spi_start_transfer(priv);
	}
}

static void ambarella_spi_prepare_transfer(struct ambarella_spi *priv)
{
	struct spi_message		*msg;
	struct spi_transfer		*xfer;
	struct ambarella_spi_cs_config  cs_config;
	u32				ctrlr0;
	void				*wbuf, *rbuf;

	msg		= priv->c_msg;
	xfer		= list_entry(msg->transfers.next, struct spi_transfer, transfer_list);
	priv->c_xfer	= xfer;
	list_del(msg->transfers.next);

	wbuf	= (void *)xfer->tx_buf;
	rbuf	= (void *)xfer->rx_buf;
	if (priv->bpw <= 8)
		priv->len	= xfer->len;
	else
		priv->len	= xfer->len >> 1;
	priv->widx	= 0;
	priv->ridx	= 0;
	if (wbuf && !rbuf)
		priv->rw_mode = SPI_WRITE_ONLY;
	if ( !wbuf && rbuf)
		priv->rw_mode = SPI_READ_ONLY;
	if (wbuf && rbuf)
		priv->rw_mode = SPI_WRITE_READ;

	ctrlr0	= amba_readl(priv->regbase + SPI_CTRLR0_OFFSET);
	ctrlr0	&= 0xfffff4ff;
	/* Always use write & read mode due to I1 changes */
	ctrlr0	|= (SPI_WRITE_READ << 8);
	if (priv->c_dev->mode & SPI_LOOP)
		ctrlr0 |= (0x1 << 11);

	amba_writel(priv->regbase + SPI_CTRLR0_OFFSET, ctrlr0);

	if (!priv->chip_select) {
		cs_config.bus_id	= priv->c_dev->master->bus_num;
		cs_config.cs_id		= priv->c_dev->chip_select;
		cs_config.cs_num	= priv->c_dev->master->num_chipselect;
		cs_config.cs_pins	= priv->pinfo->cs_pins;
		priv->pinfo->cs_activate(&cs_config);
		priv->chip_select	= 1;
	}

#if (SPI_AHB_INSTANCES > 0)
	if (!priv->pinfo->support_dma) {
#endif
		disable_irq_nosync(priv->irq);
		amba_writel(priv->regbase + SPI_IMR_OFFSET, SPI_TXEIS_MASK);
		amba_writel(priv->regbase + SPI_SSIENR_OFFSET, 1);
		amba_writel(priv->regbase + SPI_SER_OFFSET, 0);
#if (SPI_AHB_INSTANCES > 0)
	} else {
		ambarella_dma_disable_irq(SPDIF_AHB_SSI_DMA_CHAN, ambarella_spi_dma_complete);
		amba_writel(priv->regbase + SPI_DMAC_OFFSET, 0);
		amba_writel(priv->regbase + SPI_SER_OFFSET, 0);
		amba_writel(priv->regbase + SPI_TXFTLR_OFFSET, priv->pinfo->fifo_entries / 4 - 1);
		if (priv->bpw <= 8) {
			amba_writel(priv->regbase + SPI_RXFTLR_OFFSET, 8 - 1);
		} else {
			amba_writel(priv->regbase + SPI_RXFTLR_OFFSET, 4 - 1);
		}
		amba_writel(priv->regbase + SPI_IMR_OFFSET, SPI_TXEIS_MASK | SPI_TXOIS_MASK);
		amba_writel(priv->regbase + SPI_SSIENR_OFFSET, 1);
	}
#endif

}

static void ambarella_spi_finish_transfer(struct ambarella_spi *priv)
{
	if (priv->c_xfer->cs_change) {
		struct ambarella_spi_cs_config  cs_config;

		cs_config.bus_id	= priv->c_dev->master->bus_num;
		cs_config.cs_id		= priv->c_msg->spi->chip_select;
		cs_config.cs_num	= priv->c_dev->master->num_chipselect;
		cs_config.cs_pins	= priv->pinfo->cs_pins;

		priv->pinfo->cs_deactivate(&cs_config);
		priv->chip_select	= 0;
	}
	ambarella_spi_stop(priv);

	if (list_empty(&priv->c_msg->transfers)) {
		ambarella_spi_finish_message(priv);
	} else {
		ambarella_spi_prepare_transfer(priv);
		ambarella_spi_start_transfer(priv);
	}
}

static void ambarella_spi_finish_message(struct ambarella_spi *priv)
{
	struct spi_message		*msg;
	unsigned long			flags;
	u32				message_pending;

	if (priv->chip_select) {
		struct ambarella_spi_cs_config  cs_config;

		cs_config.bus_id	= priv->c_dev->master->bus_num;
		cs_config.cs_id		= priv->c_msg->spi->chip_select;
		cs_config.cs_num	= priv->c_dev->master->num_chipselect;
		cs_config.cs_pins	= priv->pinfo->cs_pins;

		priv->pinfo->cs_deactivate(&cs_config);
		priv->chip_select	= 0;
	}

	msg			= priv->c_msg;
	msg->actual_length	= priv->c_xfer->len;
	msg->status		= 0;

	/* Next Message */
	spin_lock_irqsave(&priv->lock, flags);
	list_del_init(&msg->queue);
	if (!list_empty(&priv->queue)) {
		message_pending	= 1;
	} else {
		message_pending	= 0;
		priv->idle	= 1;
		priv->c_msg	= NULL;
		priv->c_xfer	= NULL;
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	msg->complete(msg->context);
	if (message_pending) {
		ambarella_spi_handle_message(priv);
	}
}

static void ambarella_spi_handle_message(struct ambarella_spi *priv)
{
	ambarella_spi_prepare_message(priv);
	ambarella_spi_prepare_transfer(priv);
	ambarella_spi_start_transfer(priv);
}

static void ambarella_spi_prepare_message(struct ambarella_spi *priv)
{
	u32 				ctrlr0, ssi_clk, sckdv;
	struct spi_message		*msg;
	unsigned long			flags;

	spin_lock_irqsave(&priv->lock, flags);
	msg	= list_entry(priv->queue.next, struct spi_message, queue);
	spin_unlock_irqrestore(&priv->lock, flags);
	ctrlr0	= amba_readl(priv->regbase + SPI_CTRLR0_OFFSET);

	if (msg->spi->bits_per_word < 4)
		msg->spi->bits_per_word		= 4;
	if (msg->spi->bits_per_word > 16)
		msg->spi->bits_per_word		= 16;
	priv->bpw	= msg->spi->bits_per_word;

	ctrlr0	&= 0xfffffff0;
	ctrlr0	|= (priv->bpw - 1);

	ctrlr0	&= (~((1 << 6) | (1 << 7)));
	ctrlr0	|= ((msg->spi->mode & (SPI_CPHA | SPI_CPOL)) << 6);
	if (msg->spi->mode & SPI_LOOP) {
		ctrlr0 |= 0x00000800;
	}
	amba_writel(priv->regbase + SPI_CTRLR0_OFFSET, ctrlr0);

	ssi_clk	= priv->pinfo->get_ssi_freq_hz();
	if(msg->spi->max_speed_hz == 0 || msg->spi->max_speed_hz > ssi_clk / 2)
	    msg->spi->max_speed_hz = ssi_clk / 2;
	sckdv = (u16)(((ssi_clk / msg->spi->max_speed_hz) + 0x01) & 0xfffe);
	amba_writel(priv->regbase + SPI_BAUDR_OFFSET, sckdv);

	priv->chip_select	= 0;
	priv->c_dev		= msg->spi;
	priv->c_msg		= msg;
}

static int ambarella_spi_main_entry(struct spi_device *spi, struct spi_message *msg)
{
	struct ambarella_spi		*priv;
	struct spi_transfer		*xfer;
	unsigned long			flags;
	u32				shut_down, bus_idle;

	priv		= spi_master_get_devdata(spi->master);
	spin_lock_irqsave(&priv->lock, flags);
	shut_down	= priv->shutdown;
	spin_unlock_irqrestore(&priv->lock, flags);
	if (shut_down) {
		return -ESHUTDOWN;
	}

	/* Validation */
	if (list_empty(&msg->transfers) || !spi->max_speed_hz) {
		return -EINVAL;
	}

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		if (!xfer->tx_buf && !xfer->rx_buf) {
			return -EINVAL;
		}

		if (spi->bits_per_word > 8 && (xfer->len & 0x1)) {
			return -EINVAL;
		}
	}

	/* Queue Message */
	msg->status		= -EINPROGRESS;
	msg->actual_length	= 0;
	spin_lock_irqsave(&priv->lock, flags);
	list_add_tail(&msg->queue, &priv->queue);
	if (priv->idle) {
		priv->idle	= 0;
		bus_idle	= 1;
	} else {
		bus_idle	= 0;
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	/* Handle message right away if bus is idle */
	if (bus_idle) {
		ambarella_spi_handle_message(priv);
	}

	return 0;
}

static void ambarella_spi_cleanup(struct spi_device *spi)
{
	return;
}

static int ambarella_spi_inithw(struct ambarella_spi *priv)
{
	u16 				sckdv, i;
	u32 				ctrlr0, ssi_freq;

	/* Set PLL */
	if (priv->pinfo->rct_set_ssi_pll)
		priv->pinfo->rct_set_ssi_pll();

	/* Disable SPI */
	ambarella_spi_stop(priv);

	/* Config Chip Select Pins */
	for (i = 0; i < priv->pinfo->cs_num; i++) {
		if (priv->pinfo->cs_pins[i] < 0) {
			continue;
		}

		ambarella_gpio_config(priv->pinfo->cs_pins[i], GPIO_FUNC_SW_OUTPUT);
		ambarella_gpio_set(priv->pinfo->cs_pins[i], GPIO_HIGH);
	}

	/* Initial Register Settings */
	ctrlr0 = ( ( SPI_CFS << 12) | (SPI_WRITE_ONLY << 8) | (SPI_SCPOL << 7) |
		(SPI_SCPH << 6)	| (SPI_FRF << 4) | (SPI_DFS)
	      );
	amba_writel(priv->regbase + SPI_CTRLR0_OFFSET, ctrlr0);

	ssi_freq = 54000000;
	if (priv->pinfo->get_ssi_freq_hz)
		ssi_freq = priv->pinfo->get_ssi_freq_hz();
	sckdv =	(u16)(((ssi_freq / SPI_BAUD_RATE) + 0x01) & 0xfffe);
	amba_writel(priv->regbase + SPI_BAUDR_OFFSET, sckdv);

	amba_writel(priv->regbase + SPI_TXFTLR_OFFSET, 0);
	amba_writel(priv->regbase + SPI_RXFTLR_OFFSET, 1);

	return 0;
}

static irqreturn_t ambarella_spi_isr(int irq, void *dev_data)
{
	struct ambarella_spi		*priv	= dev_data;

	if (amba_readl(priv->regbase + SPI_ISR_OFFSET)) {
		disable_irq_nosync(priv->irq);
		amba_writel(priv->regbase + SPI_ISR_OFFSET, 0);

		ambarella_spi_tasklet((unsigned long)priv);
	}

	return IRQ_HANDLED;
}

#if (SPI_AHB_INSTANCES > 0)
static void ambarella_spi_dma_complete(void *dev_id, u32 dma_status)
{
	struct ambarella_spi	*priv	= dev_id;

	if (priv->rw_mode == SPI_WRITE_READ || priv->rw_mode == SPI_READ_ONLY) {
		ambcache_inv_range(priv->c_xfer->rx_buf, priv->c_xfer->len);
	}

	ambarella_spi_finish_transfer(priv);
}
#endif

static int __devinit ambarella_spi_probe(struct platform_device *pdev)
{
	struct ambarella_spi			*priv;
	struct ambarella_spi_private		*ps;
	struct spi_master			*master;
	struct spi_device 			*spidev;
	struct resource 			*res;
	struct ambarella_spi_platform_info	*pinfo;
	int					i, irq, errorCode;

	/* Get IRQ NO. */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		errorCode = -EINVAL;
		goto ambarella_spi_probe_exit3;
	}

	/* Get Platform Info */
	pinfo = (struct ambarella_spi_platform_info *)pdev->dev.platform_data;
	if (!pinfo) {
		errorCode = -EINVAL;
		goto ambarella_spi_probe_exit3;
	}
	if (pinfo->cs_num && !pinfo->cs_pins) {
		errorCode = -EINVAL;
		goto ambarella_spi_probe_exit3;
	}
	if (!pinfo->cs_activate || !pinfo->cs_deactivate) {
		errorCode = -EINVAL;
		goto ambarella_spi_probe_exit3;
	}
	if (!pinfo->rct_set_ssi_pll || !pinfo->get_ssi_freq_hz) {
		errorCode = -EINVAL;
		goto ambarella_spi_probe_exit3;
	}

	/* Get Base Address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		errorCode = -EINVAL;
		goto ambarella_spi_probe_exit3;
	}

	/* Alocate Master */
	master = spi_alloc_master(&pdev->dev, sizeof *priv);
	if (!master) {
		errorCode = -ENOMEM;
		goto ambarella_spi_probe_exit3;
	}

	/* Initalize Device Data */
	master->bus_num		= pdev->id;
	master->num_chipselect	= pinfo->cs_num;
	master->mode_bits	= SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST | SPI_LOOP;
	master->setup		= ambarella_spi_setup;
	master->transfer	= ambarella_spi_main_entry;
	master->cleanup		= ambarella_spi_cleanup;
	platform_set_drvdata(pdev, master);
	priv			= spi_master_get_devdata(master);
	priv->regbase		= (u32)res->start;
	priv->irq		= irq;
	priv->pinfo		= pinfo;
#if (SPI_AHB_INSTANCES > 0)
	if (!pinfo->support_dma) {
#endif
		tasklet_init(&priv->tasklet, ambarella_spi_tasklet, (unsigned long)priv);
#if (SPI_AHB_INSTANCES > 0)
	}
#endif
	INIT_LIST_HEAD(&priv->queue);
	priv->idle		= 1;
	priv->c_dev		= NULL;
	priv->c_msg		= NULL;
	priv->c_xfer		= NULL;
	priv->shutdown		= 0;
	spin_lock_init(&priv->lock);
	priv->bpw		= 16;

	/* Inittialize Hardware*/
	ambarella_spi_inithw(priv);

	/* Request IRQ */
#if (SPI_AHB_INSTANCES > 0)
	if (!pinfo->support_dma) {
#endif
		errorCode = request_irq(irq, ambarella_spi_isr, IRQF_TRIGGER_HIGH,
				dev_name(&pdev->dev), priv);
#if (SPI_AHB_INSTANCES > 0)
	} else {
		errorCode = ambarella_dma_request_irq(SPDIF_AHB_SSI_DMA_CHAN,
			ambarella_spi_dma_complete, priv);
	}
#endif
	if (errorCode)
		goto ambarella_spi_probe_exit2;
	else
		dev_info(&pdev->dev, "ambarella SPI Controller %d created \n", pdev->id);

	/* Register Master */
	errorCode = spi_register_master(master);
	if (errorCode)
		goto ambarella_spi_probe_exit1;

	/* Allocate Private Devices */
	ps = (struct ambarella_spi_private *)kmalloc(master->num_chipselect * \
		sizeof(struct ambarella_spi_private), GFP_KERNEL);
	if (!ps) {
		errorCode = -ENOMEM;
		goto ambarella_spi_probe_exit3;
	}
	spidev = (struct spi_device *)kmalloc(master->num_chipselect * \
		sizeof(struct spi_device), GFP_KERNEL);
	if (!spidev) {
		errorCode = -ENOMEM;
		kfree(ps);
		goto ambarella_spi_probe_exit3;
	}

	for (i = 0; i < master->num_chipselect; i++) {
		ps[i].spi		= spidev + i;
		ps[i].spi->master	= master;
		mutex_init(&ps[i].mtx);
		spin_lock_init(&ps[i].lock);
	}
	ambarella_spi_private_devices[master->bus_num].cs_num	= master->num_chipselect;
	ambarella_spi_private_devices[master->bus_num].data	= ps;

	goto ambarella_spi_probe_exit3;

ambarella_spi_probe_exit1:
#if (SPI_AHB_INSTANCES > 0)
	if (!pinfo->support_dma) {
#endif
		free_irq(irq, priv);
#if (SPI_AHB_INSTANCES > 0)
	} else {
		ambarella_dma_free_irq(SPDIF_AHB_SSI_DMA_CHAN, ambarella_spi_dma_complete);
	}
#endif

ambarella_spi_probe_exit2:
#if (SPI_AHB_INSTANCES > 0)
	if (!pinfo->support_dma) {
#endif
		tasklet_kill(&priv->tasklet);
#if (SPI_AHB_INSTANCES > 0)
	}
#endif
	spi_master_put(master);

ambarella_spi_probe_exit3:
	return errorCode;
}

static int __devexit ambarella_spi_remove(struct platform_device *pdev)
{

	struct spi_master		*master = platform_get_drvdata(pdev);
	struct ambarella_spi		*priv = spi_master_get_devdata(master);
	struct spi_message		*msg;
	unsigned long			flags;

	spin_lock_irqsave(&priv->lock, flags);
	priv->shutdown	= 1;
	spin_unlock_irqrestore(&priv->lock, flags);

#if (SPI_AHB_INSTANCES > 0)
	if (!priv->pinfo->support_dma) {
#endif
		tasklet_kill(&priv->tasklet);
#if (SPI_AHB_INSTANCES > 0)
	}
#endif
#if (SPI_AHB_INSTANCES > 0)
	if (!priv->pinfo->support_dma) {
#endif
		free_irq(priv->irq, priv);
#if (SPI_AHB_INSTANCES > 0)
	} else {
		ambarella_dma_free_irq(SPDIF_AHB_SSI_DMA_CHAN, ambarella_spi_dma_complete);
	}
#endif
	ambarella_spi_stop(priv);

	spin_lock_irqsave(&priv->lock, flags);
	list_for_each_entry(msg, &priv->queue, queue) {
		msg->status	= -ESHUTDOWN;
		msg->complete(msg->context);
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	spi_unregister_master(master);

	return 0;
}

#ifdef CONFIG_PM
static int ambarella_spi_suspend_noirq(struct device *dev)
{
	int				errorCode = 0;
	struct spi_master		*master;
	struct ambarella_spi		*priv;
	struct platform_device		*pdev;

	pdev = to_platform_device(dev);
	master = platform_get_drvdata(pdev);
	priv = spi_master_get_devdata(master);

	if (priv) {
		//disable_irq(priv->irq);
		ambarella_spi_stop(priv);
	} else {
		dev_err(&pdev->dev, "Cannot find valid pinfo\n");
		errorCode = -ENXIO;
	}

	dev_dbg(&pdev->dev, "%s\n", __func__);

	return errorCode;
}

static int ambarella_spi_resume_noirq(struct device *dev)
{
	int				errorCode = 0;
	struct spi_master		*master;
	struct ambarella_spi		*priv;
	struct platform_device		*pdev;

	pdev = to_platform_device(dev);
	master = platform_get_drvdata(pdev);
	priv = spi_master_get_devdata(master);
	if (priv) {
		ambarella_spi_inithw(priv);
		//enable_irq(priv->irq);
	} else {
		dev_err(&pdev->dev, "Cannot find valid pinfo\n");
		errorCode = -ENXIO;
	}

	dev_dbg(&pdev->dev, "%s\n", __func__);

	return errorCode;
}

static const struct dev_pm_ops ambarella_spi_dev_pm_ops = {
	.suspend_noirq = ambarella_spi_suspend_noirq,
	.resume_noirq = ambarella_spi_resume_noirq,
};
#endif

static struct platform_driver ambarella_spi_driver = {
	.probe		= ambarella_spi_probe,
	.remove		= __devexit_p(ambarella_spi_remove),
	.driver		= {
		.name	= "ambarella-spi",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &ambarella_spi_dev_pm_ops,
#endif
	},
};

static int __init ambarella_spi_init(void)
{
	return platform_driver_register(&ambarella_spi_driver);
}

static void __exit ambarella_spi_exit(void)
{
	platform_driver_unregister(&ambarella_spi_driver);
}

subsys_initcall(ambarella_spi_init);
module_exit(ambarella_spi_exit);

MODULE_DESCRIPTION("Ambarella Media Processor SPI Bus Controller");
MODULE_AUTHOR("Louis Sun, <louis.sun@ambarella.com>");
MODULE_LICENSE("GPL");


/*=================Utilities for Non-GPL Use==================================*/
static void ambarella_spi_complete(void *arg)
{
	complete(arg);
}

int ambarella_spi_write(amba_spi_cfg_t *spi_cfg, amba_spi_write_t *spi_write)
{
	u8				bus_id, cs_id, cs_num;
	int				errorCode;
	struct ambarella_spi_private	*ps;
	struct spi_device		*spi;
	struct spi_message		msg;
	struct spi_transfer		xfer;

	DECLARE_COMPLETION_ONSTACK(done);

	/* Validate Input Args */
	if (!spi_cfg || !spi_write)
		return -EINVAL;

	bus_id	= spi_write->bus_id;
	cs_id	= spi_write->cs_id;
	cs_num	= ambarella_spi_private_devices[bus_id].cs_num;
	ps	= ambarella_spi_private_devices[bus_id].data;

	if (bus_id >= SPI_MASTER_INSTANCES	|| cs_id >= cs_num
		|| !spi_write->buffer	|| !spi_write->n_size)
		return -EINVAL;

	/* Transfer */
	memset(&xfer, 0, sizeof(struct spi_transfer));
	xfer.tx_buf	= spi_write->buffer;
	xfer.len	= spi_write->n_size;
	xfer.cs_change	= spi_cfg->cs_change;

	/* Message */
	memset(&msg, 0, sizeof(struct spi_message));
	INIT_LIST_HEAD(&msg.transfers);
	list_add_tail(&xfer.transfer_list, &msg.transfers);
	msg.complete	= ambarella_spi_complete;
	msg.context	= &done;
	spi		= ps[cs_id].spi;
	msg.spi		= spi;

	mutex_lock(&ps[cs_id].mtx);

	/* Config */
	spi->chip_select	= cs_id;
	spi->mode		= spi_cfg->spi_mode;
	if (spi_cfg->lsb_first)
		spi->mode |= SPI_LSB_FIRST;
	else
		spi->mode &= ~SPI_LSB_FIRST;
	spi->mode		&= ~SPI_LOOP;
	spi->bits_per_word	= spi_cfg->cfs_dfs;
	spi->max_speed_hz	= spi_cfg->baud_rate;

	/* Wait */
	spin_lock_irq(&ps[cs_id].lock);
	errorCode = spi->master->transfer(spi, &msg);
	spin_unlock_irq(&ps[cs_id].lock);
	if (!errorCode)
		wait_for_completion(&done);

	mutex_unlock(&ps[cs_id].mtx);

	return errorCode;
}
EXPORT_SYMBOL(ambarella_spi_write);

int ambarella_spi_read(amba_spi_cfg_t *spi_cfg, amba_spi_read_t *spi_read)
{
	u8				bus_id, cs_id, cs_num;
	int				errorCode;
	struct ambarella_spi_private	*ps;
	struct spi_device		*spi;
	struct spi_message		msg;
	struct spi_transfer		xfer;

	DECLARE_COMPLETION_ONSTACK(done);

	/* Validate Input Args */
	if (!spi_cfg || !spi_read)
		return -EINVAL;

	bus_id	= spi_read->bus_id;
	cs_id	= spi_read->cs_id;
	cs_num	= ambarella_spi_private_devices[bus_id].cs_num;
	ps	= ambarella_spi_private_devices[bus_id].data;

	if (bus_id >= SPI_MASTER_INSTANCES	|| cs_id >= cs_num
		|| !spi_read->buffer	|| !spi_read->n_size)
		return -EINVAL;

	/* Transfer */
	memset(&xfer, 0, sizeof(struct spi_transfer));
	xfer.rx_buf	= spi_read->buffer;
	xfer.len	= spi_read->n_size;
	xfer.cs_change	= spi_cfg->cs_change;

	/* Message */
	memset(&msg, 0, sizeof(struct spi_message));
	INIT_LIST_HEAD(&msg.transfers);
	list_add_tail(&xfer.transfer_list, &msg.transfers);
	msg.complete	= ambarella_spi_complete;
	msg.context	= &done;
	spi		= ps[cs_id].spi;
	msg.spi		= spi;

	mutex_lock(&ps[cs_id].mtx);

	/* Config */
	spi->chip_select	= cs_id;
	spi->mode		= spi_cfg->spi_mode;
	if (spi_cfg->lsb_first)
		spi->mode |= SPI_LSB_FIRST;
	else
		spi->mode &= ~SPI_LSB_FIRST;
	spi->mode		&= ~SPI_LOOP;
	spi->bits_per_word	= spi_cfg->cfs_dfs;
	spi->max_speed_hz	= spi_cfg->baud_rate;

	/* Wait */
	spin_lock_irq(&ps[cs_id].lock);
	errorCode = spi->master->transfer(spi, &msg);
	spin_unlock_irq(&ps[cs_id].lock);
	if (!errorCode)
		wait_for_completion(&done);

	mutex_unlock(&ps[cs_id].mtx);

	return errorCode;
}
EXPORT_SYMBOL(ambarella_spi_read);

int ambarella_spi_write_then_read(amba_spi_cfg_t *spi_cfg,
	amba_spi_write_then_read_t *spi_write_then_read)
{
	u8				bus_id, cs_id, cs_num, *buf;
	u16				size;
	int				errorCode;
	struct ambarella_spi_private	*ps;
	struct spi_device		*spi;
	struct spi_message		msg;
	struct spi_transfer		xfer;

	DECLARE_COMPLETION_ONSTACK(done);

	/* Validate Input Args */
	if (!spi_cfg || !spi_write_then_read)
		return -EINVAL;

	bus_id	= spi_write_then_read->bus_id;
	cs_id	= spi_write_then_read->cs_id;
	cs_num	= ambarella_spi_private_devices[bus_id].cs_num;
	ps	= ambarella_spi_private_devices[bus_id].data;

	if (bus_id >= SPI_MASTER_INSTANCES		  || cs_id >= cs_num
		|| !spi_write_then_read->w_buffer || !spi_write_then_read->w_size
		|| !spi_write_then_read->r_buffer || !spi_write_then_read->r_size)
		return -EINVAL;

	/* Prepare Buffer */
	size	= spi_write_then_read->w_size + spi_write_then_read->r_size;
	buf	= (u8 *)kmalloc(size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, spi_write_then_read->w_buffer, spi_write_then_read->w_size);
	memset(buf + spi_write_then_read->w_size, SPI_DUMMY_DATA,
		spi_write_then_read->r_size);

	/* Transfer */
	memset(&xfer, 0, sizeof(struct spi_transfer));
	xfer.tx_buf	= buf;
	xfer.rx_buf	= buf;
	xfer.len	= size;
	xfer.cs_change	= spi_cfg->cs_change;

	/* Message */
	memset(&msg, 0, sizeof(struct spi_message));
	INIT_LIST_HEAD(&msg.transfers);
	list_add_tail(&xfer.transfer_list, &msg.transfers);
	msg.complete	= ambarella_spi_complete;
	msg.context	= &done;
	spi		= ps[cs_id].spi;
	msg.spi		= spi;

	mutex_lock(&ps[cs_id].mtx);

	/* Config */
	spi->chip_select	= cs_id;
	spi->mode		= spi_cfg->spi_mode;
	if (spi_cfg->lsb_first)
		spi->mode |= SPI_LSB_FIRST;
	else
		spi->mode &= ~SPI_LSB_FIRST;
	spi->mode		&= ~SPI_LOOP;
	spi->bits_per_word	= spi_cfg->cfs_dfs;
	spi->max_speed_hz	= spi_cfg->baud_rate;

	/* Wait */
	spin_lock_irq(&ps[cs_id].lock);
	errorCode = spi->master->transfer(spi, &msg);
	spin_unlock_irq(&ps[cs_id].lock);
	if (!errorCode)
		wait_for_completion(&done);

	mutex_unlock(&ps[cs_id].mtx);

	/* Free Buffer */
	memcpy(spi_write_then_read->r_buffer, buf + spi_write_then_read->w_size,
		spi_write_then_read->r_size);
	kfree(buf);

	return errorCode;
}
EXPORT_SYMBOL(ambarella_spi_write_then_read);

int ambarella_spi_write_and_read(amba_spi_cfg_t *spi_cfg,
	amba_spi_write_and_read_t *spi_write_and_read)
{
	u8				bus_id, cs_id, cs_num;
	int				errorCode;
	struct ambarella_spi_private	*ps;
	struct spi_device		*spi;
	struct spi_message		msg;
	struct spi_transfer		xfer;

	DECLARE_COMPLETION_ONSTACK(done);

	/* Validate Input Args */
	if (!spi_cfg || !spi_write_and_read)
		return -EINVAL;

	bus_id	= spi_write_and_read->bus_id;
	cs_id	= spi_write_and_read->cs_id;
	cs_num	= ambarella_spi_private_devices[bus_id].cs_num;
	ps	= ambarella_spi_private_devices[bus_id].data;

	if (bus_id >= SPI_MASTER_INSTANCES		|| cs_id >= cs_num
		|| !spi_write_and_read->w_buffer|| !spi_write_and_read->r_buffer
		|| !spi_write_and_read->n_size)
		return -EINVAL;

	/* Transfer */
	memset(&xfer, 0, sizeof(struct spi_transfer));
	xfer.tx_buf	= spi_write_and_read->w_buffer;
	xfer.rx_buf	= spi_write_and_read->r_buffer;
	xfer.len	= spi_write_and_read->n_size;
	xfer.cs_change	= spi_cfg->cs_change;

	/* Message */
	memset(&msg, 0, sizeof(struct spi_message));
	INIT_LIST_HEAD(&msg.transfers);
	list_add_tail(&xfer.transfer_list, &msg.transfers);
	msg.complete	= ambarella_spi_complete;
	msg.context	= &done;
	spi		= ps[cs_id].spi;
	msg.spi		= spi;

	mutex_lock(&ps[cs_id].mtx);

	/* Config */
	spi->chip_select	= cs_id;
	spi->mode		= spi_cfg->spi_mode;
	if (spi_cfg->lsb_first)
		spi->mode |= SPI_LSB_FIRST;
	else
		spi->mode &= ~SPI_LSB_FIRST;
	spi->mode		&= ~SPI_LOOP;
	spi->bits_per_word	= spi_cfg->cfs_dfs;
	spi->max_speed_hz	= spi_cfg->baud_rate;

	/* Wait */
	spin_lock_irq(&ps[cs_id].lock);
	errorCode = spi->master->transfer(spi, &msg);
	spin_unlock_irq(&ps[cs_id].lock);
	if (!errorCode)
		wait_for_completion(&done);

	mutex_unlock(&ps[cs_id].mtx);

	return errorCode;
}
EXPORT_SYMBOL(ambarella_spi_write_and_read);

