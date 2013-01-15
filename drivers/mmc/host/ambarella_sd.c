/*
 * drivers/mmc/host/ambarella_sd.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/scatterlist.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>

#include <asm/dma.h>

#include <mach/hardware.h>
#include <plat/sd.h>
#include <plat/ambcache.h>

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
#include <mach/boss.h>
#include <linux/aipc/i_sdresp.h>

/**
 * The slot IDs
 */
#define SD_HOST1_SD	0
#define SD_HOST2_SD	1
#define SD_HOST1_SDIO	2
#define SD_MAX_CARD	3

static struct ipc_sdinfo G_ipc_sdinfo[SD_MAX_CARD];
static struct mmc_host *G_mmc[SD_MAX_CARD];

#endif
/* ==========================================================================*/
#define CONFIG_SD_AMBARELLA_TIMEOUT_VAL		(0xe)
#define CONFIG_SD_AMBARELLA_WAIT_TIMEOUT	(HZ)
#define CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT	(100000)
#define CONFIG_SD_AMBARELLA_SLEEP_COUNTER_LIMIT	(1000)

#define ambsd_printk(level, phcinfo, format, arg...)	\
	printk(level "%s.%d: " format,			\
	dev_name(((struct ambarella_sd_controller_info *)phcinfo->pinfo)->dev),\
	phcinfo->slot_id, ## arg)

#define ambsd_err(phcinfo, format, arg...)		\
	ambsd_printk(KERN_ERR, phcinfo, format, ## arg)
#define ambsd_warn(phcinfo, format, arg...)		\
	ambsd_printk(KERN_WARNING, phcinfo, format, ## arg)

#ifdef CONFIG_SD_AMBARELLA_DEBUG
#define ambsd_dbg(phcinfo, format, arg...)		\
	ambsd_printk(KERN_DEBUG, phcinfo, format, ## arg)
#else
#define ambsd_dbg(phcinfo, format, arg...)		\
	({ if (0) ambsd_printk(KERN_DEBUG, phcinfo, format, ##arg); 0; })
#endif

#undef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD

/* ==========================================================================*/
enum ambarella_sd_state {
	AMBA_SD_STATE_IDLE,
	AMBA_SD_STATE_CMD,
	AMBA_SD_STATE_DATA,
	AMBA_SD_STATE_RESET,
	AMBA_SD_STATE_ERR
};

struct ambarella_sd_mmc_info {
	struct mmc_host			*mmc;
	struct mmc_request		*mrq;

	wait_queue_head_t		wait;

	enum ambarella_sd_state		state;

	struct scatterlist		*sg;
	u32				sg_len;
	u32				sg_index;
	u8				tmo;
	u32				blk_sz;
	u16				blk_cnt;
	u32				arg_reg;
	u16				xfr_reg;
	u16				cmd_reg;

	char				*buf_vaddress;
	dma_addr_t			buf_paddress;
	u32				dma_address;
	u32				dma_left;
	u32				dma_need_fill;
	u32				dma_size;
	u32				dma_per_size;

	u32				dma_w_fill_counter;
	u32				dma_w_counter;
	u32				dma_r_fill_counter;
	u32				dma_r_counter;

	void				(*pre_dma)(void *data);
	void				(*post_dma)(void *data);

	struct ambarella_sd_slot	*plat_info;
	u32				slot_id;
	void				*pinfo;
	u32				valid;
	u16				nisen;
	u32				max_blk_sz;

	struct notifier_block		system_event;
	struct semaphore		system_event_sem;
};

struct ambarella_sd_controller_info {
	unsigned char __iomem 		*regbase;
	struct device			*dev;
	struct resource			*mem;
	unsigned int			irq;
	spinlock_t			lock;
#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	unsigned int			id;
#endif

	struct ambarella_sd_controller	*pcontroller;
	struct ambarella_sd_mmc_info	*pslotinfo[SD_MAX_SLOT_NUM];
	struct mmc_ios			controller_ios;
};

/* ==========================================================================*/
static void ambarella_sd_show_info(struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s\n", __func__);
	ambsd_dbg(pslotinfo, "sg = 0x%x.\n", (u32)pslotinfo->sg);
	ambsd_dbg(pslotinfo, "sg_len = 0x%x.\n", pslotinfo->sg_len);
	ambsd_dbg(pslotinfo, "sg_index = 0x%x.\n", pslotinfo->sg_index);
	ambsd_dbg(pslotinfo, "tmo = 0x%x.\n", pslotinfo->tmo);
	ambsd_dbg(pslotinfo, "blk_sz = 0x%x.\n", pslotinfo->blk_sz);
	ambsd_dbg(pslotinfo, "blk_cnt = 0x%x.\n", pslotinfo->blk_cnt);
	ambsd_dbg(pslotinfo, "arg_reg = 0x%x.\n", pslotinfo->arg_reg);
	ambsd_dbg(pslotinfo, "xfr_reg = 0x%x.\n", pslotinfo->xfr_reg);
	ambsd_dbg(pslotinfo, "cmd_reg = 0x%x.\n", pslotinfo->cmd_reg);
	ambsd_dbg(pslotinfo, "buf_vaddress = 0x%x.\n",
		(u32)pslotinfo->buf_vaddress);
	ambsd_dbg(pslotinfo, "buf_paddress = 0x%x.\n", pslotinfo->buf_paddress);
	ambsd_dbg(pslotinfo, "dma_address = 0x%x.\n", pslotinfo->dma_address);
	ambsd_dbg(pslotinfo, "dma_left = 0x%x.\n", pslotinfo->dma_left);
	ambsd_dbg(pslotinfo, "dma_need_fill = 0x%x.\n",
		pslotinfo->dma_need_fill);
	ambsd_dbg(pslotinfo, "dma_size = 0x%x.\n", pslotinfo->dma_size);
	ambsd_dbg(pslotinfo, "dma_per_size = 0x%x.\n", pslotinfo->dma_per_size);
	ambsd_dbg(pslotinfo, "dma_w_fill_counter = 0x%x.\n",
		pslotinfo->dma_w_fill_counter);
	ambsd_dbg(pslotinfo, "dma_w_counter = 0x%x.\n",
		pslotinfo->dma_w_counter);
	ambsd_dbg(pslotinfo, "dma_r_fill_counter = 0x%x.\n",
		pslotinfo->dma_r_fill_counter);
	ambsd_dbg(pslotinfo, "dma_r_counter = 0x%x.\n",
		pslotinfo->dma_r_counter);
	ambsd_dbg(pslotinfo, "pre_dma = 0x%x.\n", (u32)pslotinfo->pre_dma);
	ambsd_dbg(pslotinfo, "post_dma = 0x%x.\n", (u32)pslotinfo->post_dma);
	ambsd_dbg(pslotinfo, "SD: state = 0x%x.\n", pslotinfo->state);
	ambsd_dbg(pslotinfo, "Exit %s\n", __func__);
}

static u32 ambarella_sd_check_dma_boundary(
	struct ambarella_sd_mmc_info *pslotinfo,
	u32 address, u32 size, u32 max_size)
{
	u32					start_512kb;
	u32					end_512kb;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	start_512kb = (address) & (~(max_size - 1));
	end_512kb = (address + size - 1) & (~(max_size - 1));

	if (start_512kb != end_512kb) {
		ambsd_dbg(pslotinfo, "Request crosses 512KB DMA boundary!\n");
		return 0;
	}

	return 1;
}

static u32 ambarella_sd_get_dma_size(u32 address)
{
	u32					dma_size = 0x80000;

	if ((address & 0x7FFFF) == 0) {
		dma_size = 0x80000;
	} else if ((address & 0x3FFFF) == 0) {
		dma_size = 0x40000;
	} else if ((address & 0x1FFFF) == 0) {
		dma_size = 0x20000;
	} else if ((address & 0xFFFF) == 0) {
		dma_size = 0x10000;
	} else if ((address & 0x7FFF) == 0) {
		dma_size = 0x8000;
	} else if ((address & 0x3FFF) == 0) {
		dma_size = 0x4000;
	} else if ((address & 0x1FFF) == 0) {
		dma_size = 0x2000;
	} else {
		dma_size = 0x1000;
	}

	return dma_size;
}

static u32 ambarella_sd_dma_size_to_mask(u32 size)
{
	u32					mask;

	switch (size) {
	case 0x80000:
		mask = SD_BLK_SZ_512KB;
		break;
	case 0x40000:
		mask = SD_BLK_SZ_256KB;
		break;
	case 0x20000:
		mask = SD_BLK_SZ_128KB;
		break;
	case 0x10000:
		mask = SD_BLK_SZ_64KB;
		break;
	case 0x8000:
		mask = SD_BLK_SZ_32KB;
		break;
	case 0x4000:
		mask = SD_BLK_SZ_16KB;
		break;
	case 0x2000:
		mask = SD_BLK_SZ_8KB;
		break;
	case 0x1000:
		mask = SD_BLK_SZ_4KB;
		break;
	default:
		mask = 0;
		BUG_ON(1);
		break;
	}

	return mask;
}

static u32 ambarella_sd_dma_mask_to_size(u32 mask)
{
	u32					size;

	switch (mask) {
	case SD_BLK_SZ_512KB:
		size = 0x80000;
		break;
	case SD_BLK_SZ_256KB:
		size = 0x40000;
		break;
	case SD_BLK_SZ_128KB:
		size = 0x20000;
		break;
	case SD_BLK_SZ_64KB:
		size = 0x10000;
		break;
	case SD_BLK_SZ_32KB:
		size = 0x8000;
		break;
	case SD_BLK_SZ_16KB:
		size = 0x4000;
		break;
	case SD_BLK_SZ_8KB:
		size = 0x2000;
		break;
	case SD_BLK_SZ_4KB:
		size = 0x1000;
		break;
	default:
		size = 0;
		BUG_ON(1);
		break;
	}

	return size;
}

static void ambarella_sd_pre_sg_to_dma(void *data)
{
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;
	struct scatterlist			*current_sg;
	u32					dma_start = 0xffffffff;
	u32					dma_end = 0;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	current_sg = pslotinfo->sg;
	if (pslotinfo->sg_index == 0) {
		pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
			pslotinfo->max_blk_sz);
		pslotinfo->dma_w_counter++;

		for (i = 0; i < pslotinfo->sg_len; i++) {
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
			current_sg[i].dma_address = dma_map_page(pinfo->dev,
							sg_page(&current_sg[i]),
							current_sg[i].offset,
							current_sg[i].length,
							DMA_TO_DEVICE);
#else
			current_sg[i].dma_address = sg_phys(&current_sg[i]);
			ambcache_flush_range(sg_virt(&current_sg[i]),
				current_sg[i].length);
#endif
			if ((current_sg[i].length & 0xFFF) &&
				(i < pslotinfo->sg_len - 1)) {
				ambsd_dbg(pslotinfo,
					"Short DMA length[0x%x], %d&%d\n",
					current_sg[i].length,
					i, pslotinfo->sg_len);
				pslotinfo->dma_need_fill = 1;
			}
			dma_start = min(dma_start, current_sg[i].dma_address);
			dma_end = max(dma_end, (current_sg[i].dma_address +
					current_sg[i].length));
			pslotinfo->dma_per_size = min(pslotinfo->dma_per_size,
				ambarella_sd_get_dma_size(
					current_sg[i].length));
			pslotinfo->dma_per_size = min(pslotinfo->dma_per_size,
				ambarella_sd_get_dma_size(
					current_sg[i].dma_address));
		}
		if (pslotinfo->buf_vaddress == NULL) {
			pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->max_blk_sz);
		}
		if (ambarella_sd_check_dma_boundary(pslotinfo, dma_start,
			(dma_end - dma_start), pslotinfo->dma_per_size) == 0) {
			pslotinfo->dma_need_fill = 1;
			ambsd_dbg(pslotinfo,
				"dma_start = 0x%x, dma_end = 0x%x\n",
				dma_start, dma_end);
		}

		if (pslotinfo->dma_need_fill) {
			BUG_ON((pslotinfo->buf_vaddress == NULL) ||
				(pslotinfo->buf_paddress == (dma_addr_t)NULL));
			pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->max_blk_sz);
			pslotinfo->dma_w_fill_counter++;
			sg_copy_to_buffer(current_sg, pslotinfo->sg_len,
				pslotinfo->buf_vaddress, pslotinfo->dma_size);
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
			dma_sync_single_for_cpu(
				pinfo->dev, pslotinfo->buf_paddress,
				pslotinfo->dma_size, DMA_TO_DEVICE);
#else
			ambcache_flush_range(pslotinfo->buf_vaddress,
				pslotinfo->dma_size);
#endif
			pslotinfo->dma_address = pslotinfo->buf_paddress;
			pslotinfo->dma_left = pslotinfo->dma_size;
			pslotinfo->sg_index = pslotinfo->sg_len;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |= pslotinfo->max_blk_sz;
		} else {
			pslotinfo->dma_address =
				current_sg[pslotinfo->sg_index].dma_address;
			pslotinfo->dma_left =
				current_sg[pslotinfo->sg_index].length;
			pslotinfo->sg_index++;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |= ambarella_sd_dma_size_to_mask(
				pslotinfo->dma_per_size);
		}
	} else if (pslotinfo->sg_index < pslotinfo->sg_len) {
		if (pslotinfo->dma_left) {
			pslotinfo->dma_address += pslotinfo->dma_per_size;
		} else {
			pslotinfo->dma_address =
				current_sg[pslotinfo->sg_index].dma_address;
			pslotinfo->dma_left =
				current_sg[pslotinfo->sg_index].length;
			pslotinfo->sg_index++;
		}
	} else if (pslotinfo->sg_index == pslotinfo->sg_len) {
		if (pslotinfo->dma_left) {
			pslotinfo->dma_address += pslotinfo->dma_per_size;
		} else {
			pslotinfo->dma_address = 0;
		}
	} else {
		pslotinfo->dma_address = 0;
	}

	if (pslotinfo->dma_left >= pslotinfo->dma_per_size)
		pslotinfo->dma_left -= pslotinfo->dma_per_size;
	else
		pslotinfo->dma_left = 0;

	//ambarella_sd_show_info(pslotinfo);
}

static void ambarella_sd_post_sg_to_dma(void *data)
{
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;
	struct scatterlist			*current_sg;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	current_sg = pslotinfo->sg;

	for (i = 0; i < pslotinfo->sg_len; i++) {
		dma_unmap_page(pinfo->dev, current_sg[i].dma_address,
				current_sg[i].length, DMA_TO_DEVICE);
	}
#endif
}

static void ambarella_sd_pre_dma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;
	struct scatterlist			*current_sg;
	u32					dma_start = 0xffffffff;
	u32					dma_end = 0;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	current_sg = pslotinfo->sg;

	if (pslotinfo->sg_index == 0) {
		pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
			pslotinfo->max_blk_sz);
		pslotinfo->dma_r_counter++;

		for (i = 0; i < pslotinfo->sg_len; i++) {
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
			current_sg[i].dma_address = dma_map_page(pinfo->dev,
							sg_page(&current_sg[i]),
							current_sg[i].offset,
							current_sg[i].length,
							DMA_FROM_DEVICE);
#else
			current_sg[i].dma_address = sg_phys(&current_sg[i]);
			ambcache_inv_range(sg_virt(&current_sg[i]),
				current_sg[i].length);
#endif
			if ((current_sg[i].length & 0xFFF) &&
				(i < pslotinfo->sg_len - 1)) {
				ambsd_dbg(pslotinfo,
					"Short DMA length[0x%x], %d&%d\n",
					current_sg[i].length,
					i, pslotinfo->sg_len);
				pslotinfo->dma_need_fill = 1;
			}
			dma_start = min(dma_start, current_sg[i].dma_address);
			dma_end = max(dma_end, (current_sg[i].dma_address +
					current_sg[i].length));
			pslotinfo->dma_per_size = min(pslotinfo->dma_per_size,
				ambarella_sd_get_dma_size(
					current_sg[i].length));
			pslotinfo->dma_per_size = min(pslotinfo->dma_per_size,
				ambarella_sd_get_dma_size(
					current_sg[i].dma_address));
		}
		if (pslotinfo->buf_vaddress == NULL) {
			pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->max_blk_sz);
		}
		if (ambarella_sd_check_dma_boundary(pslotinfo, dma_start,
			(dma_end - dma_start), pslotinfo->dma_per_size) == 0) {
			pslotinfo->dma_need_fill = 1;
			ambsd_dbg(pslotinfo,
				"dma_start = 0x%x, dma_end = 0x%x\n",
				dma_start, dma_end);
		}

		if (pslotinfo->dma_need_fill) {
			BUG_ON((pslotinfo->buf_vaddress == NULL) ||
				(pslotinfo->buf_paddress == (dma_addr_t)NULL));
			pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->max_blk_sz);
			pslotinfo->dma_r_fill_counter++;
			pslotinfo->dma_address = pslotinfo->buf_paddress;
			pslotinfo->dma_left = pslotinfo->dma_size;
			pslotinfo->sg_index = pslotinfo->sg_len;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |= pslotinfo->max_blk_sz;
		} else {
			pslotinfo->dma_address =
				current_sg[pslotinfo->sg_index].dma_address;
			pslotinfo->dma_left =
				current_sg[pslotinfo->sg_index].length;
			pslotinfo->sg_index++;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |= ambarella_sd_dma_size_to_mask(
				pslotinfo->dma_per_size);
		}
	} else if (pslotinfo->sg_index < pslotinfo->sg_len) {
		if (pslotinfo->dma_left) {
			pslotinfo->dma_address += pslotinfo->dma_per_size;
		} else {
			pslotinfo->dma_address =
				current_sg[pslotinfo->sg_index].dma_address;
			pslotinfo->dma_left =
				current_sg[pslotinfo->sg_index].length;
			pslotinfo->sg_index++;
		}
	} else if (pslotinfo->sg_index == pslotinfo->sg_len) {
		if (pslotinfo->dma_left) {
			pslotinfo->dma_address += pslotinfo->dma_per_size;
		} else {
			pslotinfo->dma_address = 0;
		}
	} else {
		pslotinfo->dma_address = 0;
	}

	if (pslotinfo->dma_left >= pslotinfo->dma_per_size)
		pslotinfo->dma_left -= pslotinfo->dma_per_size;
	else
		pslotinfo->dma_left = 0;

	//ambarella_sd_show_info(pslotinfo);
}

static void ambarella_sd_post_dma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;
	struct scatterlist			*current_sg;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
	current_sg = pslotinfo->sg;

	for (i = 0; i < pslotinfo->sg_len; i++) {
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
		dma_unmap_page(pinfo->dev, current_sg[i].dma_address,
				current_sg[i].length, DMA_FROM_DEVICE);
#else
		ambcache_inv_range(sg_virt(&current_sg[i]),
			current_sg[i].length);
#endif
	}
	if (pslotinfo->dma_need_fill) {
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
		dma_sync_single_for_cpu(pinfo->dev, pslotinfo->buf_paddress,
			pslotinfo->dma_size, DMA_FROM_DEVICE);
#else
		ambcache_inv_range(pslotinfo->buf_vaddress,
			pslotinfo->dma_size);
#endif
		sg_copy_from_buffer(current_sg, pslotinfo->sg_len,
			pslotinfo->buf_vaddress, pslotinfo->dma_size);
	}
}

static void ambarella_sd_request_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	down(&pslotinfo->system_event_sem);

	if (pslotinfo->plat_info->request)
		pslotinfo->plat_info->request();
}

static void ambarella_sd_release_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	if (pslotinfo->plat_info->release)
		pslotinfo->plat_info->release();

	up(&pslotinfo->system_event_sem);
}

static void ambarella_sd_enable_normal_int(struct mmc_host *mmc, u16 ints)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	unsigned long				flags;
	u16					int_flag;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	spin_lock_irqsave(&pinfo->lock, flags);

	int_flag = amba_readw(pinfo->regbase + SD_NISEN_OFFSET);
	int_flag |= ints;
	amba_writew(pinfo->regbase + SD_NISEN_OFFSET, int_flag);
	pslotinfo->nisen = int_flag;

	int_flag = amba_readw(pinfo->regbase + SD_NIXEN_OFFSET);
	int_flag |= ints;
	amba_writew(pinfo->regbase + SD_NIXEN_OFFSET, int_flag);

	spin_unlock_irqrestore(&pinfo->lock, flags);
}

static void ambarella_sd_disable_normal_int(struct mmc_host *mmc, u16 ints)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	unsigned long				flags;
	u16					int_flag;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	spin_lock_irqsave(&pinfo->lock, flags);

	int_flag = amba_readw(pinfo->regbase + SD_NISEN_OFFSET);
	int_flag &= ~ints;
	amba_writew(pinfo->regbase + SD_NISEN_OFFSET, int_flag);
	pslotinfo->nisen = int_flag;

	int_flag = amba_readw(pinfo->regbase + SD_NIXEN_OFFSET);
	int_flag &= ~ints;
	amba_writew(pinfo->regbase + SD_NIXEN_OFFSET, int_flag);

	spin_unlock_irqrestore(&pinfo->lock, flags);
}

static void ambarella_sd_enable_error_int(struct mmc_host *mmc, u16 ints)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	unsigned long				flags;
	u16					int_flag;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	spin_lock_irqsave(&pinfo->lock, flags);

	int_flag = amba_readw(pinfo->regbase + SD_EISEN_OFFSET);
	int_flag |= ints;
	amba_writew(pinfo->regbase + SD_EISEN_OFFSET, int_flag);

	int_flag = amba_readw(pinfo->regbase + SD_EIXEN_OFFSET);
	int_flag |= ints;
	amba_writew(pinfo->regbase + SD_EIXEN_OFFSET, int_flag);

	spin_unlock_irqrestore(&pinfo->lock, flags);
}

static void ambarella_sd_disable_error_int(struct mmc_host *mmc, u16 ints)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	unsigned long				flags;
	u16					int_flag;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	spin_lock_irqsave(&pinfo->lock, flags);

	int_flag = amba_readw(pinfo->regbase + SD_EISEN_OFFSET);
	int_flag &= ~ints;
	amba_writew(pinfo->regbase + SD_EISEN_OFFSET, int_flag);

	int_flag = amba_readw(pinfo->regbase + SD_EIXEN_OFFSET);
	int_flag &= ~ints;
	amba_writew(pinfo->regbase + SD_EIXEN_OFFSET, int_flag);

	spin_unlock_irqrestore(&pinfo->lock, flags);
}

static void ambarella_sd_reset_all(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	u16					int_flag = 0;
	u32					counter = 0;
	u8					reset_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s with state %d\n",
		__func__, pslotinfo->state);

	ambarella_sd_disable_normal_int(mmc, 0xFFFF);
	ambarella_sd_disable_error_int(mmc, 0xFFFF);
	amba_write2w(pinfo->regbase + SD_NIS_OFFSET, 0xFFFF, 0xFFFF);
	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_ALL);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_ALL))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_SLEEP_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_ALL....\n");
			break;
		}
		mdelay(1);
	}

	amba_writeb(pinfo->regbase + SD_TMO_OFFSET,
		CONFIG_SD_AMBARELLA_TIMEOUT_VAL);

	int_flag = pslotinfo->nisen	|
#if defined(CONFIG_AMBARELLA_IPC)
		/* Use gpio for card detection */
#else
		SD_NISEN_REMOVAL	|
		SD_NISEN_INSERT		|
#endif
		SD_NISEN_DMA		|
		SD_NISEN_BLOCK_GAP	|
		SD_NISEN_XFR_DONE	|
		SD_NISEN_CMD_DONE;
	ambarella_sd_enable_normal_int(mmc, int_flag);

	int_flag = SD_EISEN_ACMD12_ERR	|
		SD_EISEN_CURRENT_ERR	|
		SD_EISEN_DATA_BIT_ERR	|
		SD_EISEN_DATA_CRC_ERR	|
		SD_EISEN_DATA_TMOUT_ERR	|
		SD_EISEN_CMD_IDX_ERR	|
		SD_EISEN_CMD_BIT_ERR	|
		SD_EISEN_CMD_CRC_ERR	|
		SD_EISEN_CMD_TMOUT_ERR;
	ambarella_sd_enable_error_int(mmc, int_flag);

	pslotinfo->state = AMBA_SD_STATE_RESET;

	ambsd_dbg(pslotinfo, "Exit %s with counter %d\n", __func__, counter);
}

static void ambarella_sd_reset_cmd_line(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	u32					counter = 0;
	u8					reset_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s with state %d\n",
		__func__, pslotinfo->state);

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_CMD);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_CMD))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_CMD...\n");
			break;
		}
	}

	ambsd_dbg(pslotinfo, "Exit %s with counter %d\n", __func__, counter);
}

static void ambarella_sd_reset_data_line(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;
	u32					counter = 0;
	u8					reset_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s with state %d\n",
		__func__, pslotinfo->state);

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_DAT);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_DAT))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_DAT...\n");
			break;
		}
	}

	ambsd_dbg(pslotinfo, "Exit %s with counter %d\n", __func__, counter);
}

static void ambarella_sd_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	if (enable)
		ambarella_sd_enable_normal_int(mmc, SD_NISEN_CARD);
	else
		ambarella_sd_disable_normal_int(mmc, SD_NISEN_CARD);
}

static void ambarella_sd_data_done(
	struct ambarella_sd_mmc_info *pslotinfo,
	u16 nis,
	u16 eis)
{
	struct mmc_data				*data = NULL;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if ((pslotinfo->state == AMBA_SD_STATE_CMD) &&
		((pslotinfo->cmd_reg & 0x3) == SD_CMD_RSP_48BUSY)) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
		wake_up(&pslotinfo->wait);
		return;
	}

	if (!pslotinfo->mrq) {
		ambsd_err(pslotinfo, "%s mrq is NULL\n", __func__);
		return;
	}

	if (pslotinfo->mrq->cmd) {
		data = pslotinfo->mrq->cmd->data;
	} else {
		ambsd_err(pslotinfo, "%s cmd is NULL\n", __func__);
		return;
	}

	if (pslotinfo->state != AMBA_SD_STATE_DATA) {
		ambsd_err(pslotinfo, "%s wrong cmd%d state%d, 0x%08X 0x%08X\n",
			__func__, pslotinfo->mrq->cmd->opcode,
			pslotinfo->state, nis, eis);
		return;
	}

	if (data == NULL) {
		ambsd_err(pslotinfo, "%s data is NULL\n", __func__);
		return;
	}

	if (eis != 0x0) {
		if (eis & SD_EIS_DATA_BIT_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_DATA_CRC_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_DATA_TMOUT_ERR) {
			data->error = -ETIMEDOUT;
		} else {
			data->error = -EIO;
		}
		ambarella_sd_reset_data_line(pslotinfo->mmc);
	} else {
		pslotinfo->pre_dma(pslotinfo);
		BUG_ON(pslotinfo->dma_address != 0);
		data->bytes_xfered = pslotinfo->dma_size;
	}

	pslotinfo->post_dma(pslotinfo);
	pslotinfo->state = AMBA_SD_STATE_IDLE;
	wake_up(&pslotinfo->wait);
}

static void ambarella_sd_cmd_done(
	struct ambarella_sd_mmc_info *pslotinfo,
	u16 nis,
	u16 eis)
{
	struct mmc_command			*cmd = NULL;
	u32					rsp0, rsp1, rsp2, rsp3;
	struct ambarella_sd_controller_info	*pinfo;
	u16					ac12es;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (!pslotinfo->mrq) {
		ambsd_err(pslotinfo, "%s mrq is NULL\n", __func__);
		return;
	}

	if (pslotinfo->mrq->cmd) {
		cmd = pslotinfo->mrq->cmd;
	} else {
		ambsd_err(pslotinfo, "%s cmd is NULL\n", __func__);
		return;
	}

	if (eis != 0x0) {
		if (eis & SD_EIS_CMD_BIT_ERR) {
			cmd->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_CRC_ERR) {
			cmd->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_TMOUT_ERR) {
			cmd->error = -ETIMEDOUT;
		} else if (eis & SD_EIS_ACMD12_ERR) {
			ac12es = amba_readl(pinfo->regbase + SD_AC12ES_OFFSET);
			if (ac12es & SD_AC12ES_TMOUT_ERROR) {
				cmd->error = -ETIMEDOUT;
			} else if (eis & SD_AC12ES_CRC_ERROR) {
				cmd->error = -EILSEQ;
			} else {
				cmd->error = -EIO;
			}

			if (pslotinfo->mrq->stop) {
				pslotinfo->mrq->stop->error = cmd->error;
			} else {
				ambsd_err(pslotinfo, "%s NULL stop 0x%x %d\n",
					__func__, ac12es, cmd->error);
			}
		} else {
			cmd->error = -EIO;
		}
		ambarella_sd_reset_cmd_line(pslotinfo->mmc);
	}

	rsp0 = amba_readl(pinfo->regbase + SD_RSP0_OFFSET);
	rsp1 = amba_readl(pinfo->regbase + SD_RSP1_OFFSET);
	rsp2 = amba_readl(pinfo->regbase + SD_RSP2_OFFSET);
	rsp3 = amba_readl(pinfo->regbase + SD_RSP3_OFFSET);

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[0] = ((rsp3 << 8) | (rsp2 >> 24));
		cmd->resp[1] = ((rsp2 << 8) | (rsp1 >> 24));
		cmd->resp[2] = ((rsp1 << 8) | (rsp0 >> 24));
		cmd->resp[3] = (rsp0 << 8);
	} else {
		cmd->resp[0] = rsp0;
	}

	if ((pslotinfo->state == AMBA_SD_STATE_CMD) &&
		((pslotinfo->cmd_reg & 0x3) != SD_CMD_RSP_48BUSY)) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
		wake_up(&pslotinfo->wait);
	}
}

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
#define VIC2_INT_VEC_OFFSET	32
#define VIC3_INT_VEC_OFFSET	64

static inline u32 vic_check(u32 line)
{
	if (line < 32) {
		return amba_readl(VIC_INTEN_REG) & (0x1 << line);
#if (VIC_INSTANCES >= 2)
	} else if (line < VIC2_INT_VEC_OFFSET + 32) {
		u32 val = (0x1 << (line - VIC2_INT_VEC_OFFSET));
		return amba_readl(VIC2_INTEN_REG) & val;
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < VIC3_INT_VEC_OFFSET + 32) {
		u32 val = (0x1 << (line - VIC3_INT_VEC_OFFSET));
		return amba_readl(VIC3_INTEN_REG) & val;
#endif
	}
	return 0;
}

static inline void ambarella_sd_vic_ctrl(u32 line, u32 enable)
{
	if (line < 32) {
		if (enable)
			amba_writel(VIC_INTEN_REG ,(0x1 << line));
		else
			amba_writel(VIC_INTEN_CLR_REG ,(0x1 << line));
#if (VIC_INSTANCES >= 2)
	} else if (line < VIC2_INT_VEC_OFFSET + 32) {
		u32 val = (0x1 << (line - VIC2_INT_VEC_OFFSET));
		if (enable)
			amba_writel(VIC2_INTEN_REG ,val);
		else
			amba_writel(VIC2_INTEN_CLR_REG ,val);
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < VIC3_INT_VEC_OFFSET + 32) {
		u32 val = (0x1 << (line - VIC3_INT_VEC_OFFSET));
		if (enable)
			amba_writel(VIC3_INTEN_REG ,val);
		else
			amba_writel(VIC3_INTEN_CLR_REG ,val);
#endif
	}

	return;
}
#endif

static irqreturn_t ambarella_sd_irq(int irq, void *devid)
{
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo = NULL;
	u16					nis;
	u16					eis;
	u32					enabled = 0;
	u32					i;

	pinfo = (struct ambarella_sd_controller_info *)devid;

	/* Read the interrupt registers */
	amba_read2w(pinfo->regbase + SD_NIS_OFFSET, &nis, &eis);

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	if ((nis & SD_NIS_CARD) != SD_NIS_CARD &&
	    (vic_check(pinfo->irq - 32)) != 0)
		goto ambarella_sd_irq_exit;
#endif

	if (unlikely(nis == 0)) {
		goto ambarella_sd_irq_exit;
	} else if (nis & SD_NIS_CARD) {
		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			if (pslotinfo->nisen & SD_NISEN_CARD) {
				ambsd_dbg(pslotinfo, "SD_NIS_CARD\n");
				mmc_signal_sdio_irq(pslotinfo->mmc);
			}
		}
	}

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->plat_info->check_owner)
			enabled = pslotinfo->plat_info->check_owner();
		if (enabled)
			break;
	}

	if (unlikely(pslotinfo == NULL))
		goto ambarella_sd_irq_clear_irq;

	ambsd_dbg(pslotinfo, "%s nis = 0x%x, eis = 0x%x & %d:%d\n", __func__,
		nis, eis, pslotinfo->state, enabled);

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	if ((nis & SD_NIS_CMD_DONE) ||
	    (nis & SD_NIS_XFR_DONE) ||
	    (nis & SD_NIS_DMA)) {
		if (pslotinfo->mrq && pslotinfo->mrq->cmd) {
			/* Normal Interrupt from Linux */
		} else
			goto ambarella_sd_irq_exit;
	}
#endif

	/* Check for card detection interrupt */
	if (!ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd) &&
		(pslotinfo->plat_info->fixed_cd == -1)) {
		if (nis & SD_NIS_REMOVAL) {
			ambsd_dbg(pslotinfo, "SD_NIS_REMOVAL\n");
			mmc_detect_change(pslotinfo->mmc,
				pslotinfo->plat_info->cd_delay);
		} else if (nis & SD_NIS_INSERT) {
			ambsd_dbg(pslotinfo, "SD_NIS_INSERT\n");
			mmc_detect_change(pslotinfo->mmc,
				pslotinfo->plat_info->cd_delay);
		}
	}

ambarella_sd_irq_clear_irq:
	/* Clear interrupt */
	amba_write2w(pinfo->regbase + SD_NIS_OFFSET, nis, eis);

	if (!enabled)
		goto ambarella_sd_irq_exit;

	/* Check for command normal completion */
	if (nis & SD_NIS_CMD_DONE) {
		ambarella_sd_cmd_done(pslotinfo, nis, eis);
	}

	/* Check for data normal completion */
	if (nis & SD_NIS_XFR_DONE) {
		ambarella_sd_data_done(pslotinfo, nis, eis);
	}
	else /* Check for DMA interrupt */
	if (nis & SD_NIS_DMA) {
		pslotinfo->pre_dma(pslotinfo);
		if (pslotinfo->dma_address != 0) {
			amba_writel(pinfo->regbase + SD_DMA_ADDR_OFFSET,
				pslotinfo->dma_address);
		}
	}

	if (eis != 0x0) {
		if (pslotinfo->state == AMBA_SD_STATE_CMD)
			ambarella_sd_cmd_done(pslotinfo, nis, eis);
		else if (pslotinfo->state == AMBA_SD_STATE_DATA)
			ambarella_sd_data_done(pslotinfo, nis, eis);
	}

ambarella_sd_irq_exit:
	return IRQ_HANDLED;
}

static int ambarella_sd_gpio_cd_check_val(
	struct ambarella_sd_mmc_info *pslotinfo)
{
	u32					val = -1;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd)) {
		ambarella_gpio_config(pslotinfo->plat_info->gpio_cd.irq_gpio,
			GPIO_FUNC_SW_INPUT);
		val = ambarella_gpio_get(
			pslotinfo->plat_info->gpio_cd.irq_gpio);
		ambarella_gpio_config(pslotinfo->plat_info->gpio_cd.irq_gpio,
			pslotinfo->plat_info->gpio_cd.irq_gpio_mode);
		val = (val == pslotinfo->plat_info->gpio_cd.irq_gpio_val) ?
			1 : 0;
		ambsd_dbg(pslotinfo, "%s:%d\n", (val == 1) ?
			"card insert" : "card eject",
			pslotinfo->plat_info->cd_delay);
	}

	return val;
}

static irqreturn_t ambarella_sd_gpio_cd_irq(int irq, void *devid)
{
	struct ambarella_sd_mmc_info		*pslotinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)devid;

	if (pslotinfo->valid &&
		(ambarella_sd_gpio_cd_check_val(pslotinfo) != -1)) {
		mmc_detect_change(pslotinfo->mmc,
			pslotinfo->plat_info->cd_delay);
	}

	return IRQ_HANDLED;
}

static void ambarella_sd_send_cmd(
	struct ambarella_sd_mmc_info *pslotinfo,
	struct mmc_command *cmd, struct mmc_command *stop)
{
	struct mmc_data				*data;
	u32					counter = 0;
	u32					tmpreg;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	pslotinfo->state = AMBA_SD_STATE_CMD;
	data = cmd->data;

	pslotinfo->sg_len = 0;
	pslotinfo->sg_index = 0;
	pslotinfo->sg = NULL;
	pslotinfo->blk_sz = 0;
	pslotinfo->blk_cnt = 0;
	pslotinfo->arg_reg = 0;
	pslotinfo->cmd_reg = 0;
	pslotinfo->tmo = 0;

	pslotinfo->xfr_reg = 0;
	if (stop) {
		if (likely(stop->opcode == MMC_STOP_TRANSMISSION)) {
			pslotinfo->xfr_reg = SD_XFR_AC12_EN;
		} else {
			ambsd_err(pslotinfo, "%s strange stop cmd%d\n",
				__func__, stop->opcode);
		}
	}

	pslotinfo->dma_address = 0;
	pslotinfo->dma_left = 0;
	pslotinfo->dma_need_fill = 0;
	pslotinfo->dma_size = 0;
	pslotinfo->dma_per_size = 0;

	if (!(cmd->flags & MMC_RSP_PRESENT))
		pslotinfo->cmd_reg = SD_CMD_RSP_NONE;
	else if (cmd->flags & MMC_RSP_136)
		pslotinfo->cmd_reg = SD_CMD_RSP_136;
	else if (cmd->flags & MMC_RSP_BUSY)
		pslotinfo->cmd_reg = SD_CMD_RSP_48BUSY;
	else
		pslotinfo->cmd_reg = SD_CMD_RSP_48;

	if (cmd->flags & MMC_RSP_CRC)
		pslotinfo->cmd_reg |= SD_CMD_CHKCRC;

	if (cmd->flags & MMC_RSP_OPCODE)
		pslotinfo->cmd_reg |= SD_CMD_CHKIDX;

	pslotinfo->cmd_reg |= SD_CMD_IDX(cmd->opcode);
	pslotinfo->arg_reg = cmd->arg;

	if (data != NULL) {
#ifdef CONFIG_MMC_AMBARELLA_CAL_TMO
		u32 sd_clk;
		u32 clk_divisor;
		u32 tmoclk;
		u32 desired_tmo;
		u32 actual_tmo;

		sd_clk = pinfo->pcontroller->get_pll();
		clk_divisor = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		clk_divisor >>= 8;
		tmoclk = sd_clk / (0x1 << clk_divisor) / 1000;
		desired_tmo = (data->timeout_ns / 1000000) * tmoclk;
		desired_tmo += data->timeout_clks / 1000;
		for ((pslotinfo->tmo) = 0;
			(pslotinfo->tmo) < 0xe; (pslotinfo->tmo)++) {
			actual_tmo =
				(2 << (13 + (pslotinfo->tmo))) * tmoclk / 1000;
			if (actual_tmo >= desired_tmo)
				break;
		}
#else
		pslotinfo->tmo = CONFIG_SD_AMBARELLA_TIMEOUT_VAL;
#endif
		pslotinfo->state = AMBA_SD_STATE_DATA;

		pslotinfo->blk_sz = data->blksz;
		pslotinfo->dma_size = data->blksz * data->blocks;

		pslotinfo->sg_len = data->sg_len;
		pslotinfo->sg = data->sg;

		pslotinfo->xfr_reg |= SD_XFR_DMA_EN;
		pslotinfo->cmd_reg |= SD_CMD_DATA;

		pslotinfo->blk_cnt = data->blocks;
		if (pslotinfo->blk_cnt > 1) {
			pslotinfo->xfr_reg |= SD_XFR_MUL_SEL;
			pslotinfo->xfr_reg |= SD_XFR_BLKCNT_EN;
		}

		if (data->flags & MMC_DATA_STREAM) {
			pslotinfo->xfr_reg |= SD_XFR_MUL_SEL;
			pslotinfo->xfr_reg &= ~SD_XFR_BLKCNT_EN;
		}

		if (data->flags & MMC_DATA_WRITE) {
			pslotinfo->xfr_reg &= ~SD_XFR_CTH_SEL;
			pslotinfo->pre_dma = &ambarella_sd_pre_sg_to_dma;
			pslotinfo->post_dma = &ambarella_sd_post_sg_to_dma;
		} else {
			pslotinfo->xfr_reg |= SD_XFR_CTH_SEL;
			pslotinfo->pre_dma = &ambarella_sd_pre_dma_to_sg;
			pslotinfo->post_dma = &ambarella_sd_post_dma_to_sg;
		}

		pslotinfo->pre_dma(pslotinfo);

		while (1) {
			tmpreg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((tmpreg & SD_STA_CMD_INHIBIT_DAT) == 0) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				ambsd_warn(pslotinfo,
					"Wait SD_STA_CMD_INHIBIT_DAT...\n");
				pslotinfo->state = AMBA_SD_STATE_ERR;
				return;
			}
		}

		amba_writeb(pinfo->regbase + SD_TMO_OFFSET, pslotinfo->tmo);
		if (pslotinfo->dma_address == 0) {
			ambsd_warn(pslotinfo, "Wrong dma_address.\n");
			pslotinfo->state = AMBA_SD_STATE_ERR;
			return;
		}
		amba_writel(pinfo->regbase + SD_DMA_ADDR_OFFSET,
			pslotinfo->dma_address);
		amba_write2w(pinfo->regbase + SD_BLK_SZ_OFFSET,
			pslotinfo->blk_sz, pslotinfo->blk_cnt);
		amba_writel(pinfo->regbase + SD_ARG_OFFSET, pslotinfo->arg_reg);
		amba_write2w(pinfo->regbase + SD_XFR_OFFSET,
			pslotinfo->xfr_reg, pslotinfo->cmd_reg);
	} else {
		while (1) {
			tmpreg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((tmpreg & SD_STA_CMD_INHIBIT_CMD) == 0) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				ambsd_warn(pslotinfo,
					"Wait SD_STA_CMD_INHIBIT_CMD...\n");
				pslotinfo->state = AMBA_SD_STATE_ERR;
				return;
			}
		}

		amba_writel(pinfo->regbase + SD_ARG_OFFSET, pslotinfo->arg_reg);
		amba_write2w(pinfo->regbase + SD_XFR_OFFSET,
			0x00, pslotinfo->cmd_reg);
	}
	ambarella_sd_show_info(pslotinfo);
}

static void ambarella_sd_set_clk(struct mmc_host *mmc, u32 clk)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	u16					clk_div = 0x00;
	u16					clkreg;
	u32					sd_clk;
	u32					desired_clk;
	u32					actual_clk;
	struct ambarella_sd_controller_info	*pinfo;
	u32					counter = 0;
	u32					bneed_div = 1;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (pinfo->pcontroller->clk_limit > pinfo->pcontroller->max_clk)
		pinfo->pcontroller->clk_limit = pinfo->pcontroller->max_clk;

	if (clk == 0) {
		amba_writew(pinfo->regbase + SD_CLK_OFFSET, 0);
#if 0
		ambsd_dbg(pslotinfo, "dma_w_fill_counter = 0x%x.\n",
			pslotinfo->dma_w_fill_counter);
		ambsd_dbg(pslotinfo, "dma_w_counter = 0x%x.\n",
			pslotinfo->dma_w_counter);
		ambsd_dbg(pslotinfo, "dma_r_fill_counter = 0x%x.\n",
			pslotinfo->dma_r_fill_counter);
		ambsd_dbg(pslotinfo, "dma_r_counter = 0x%x.\n",
			pslotinfo->dma_r_counter);
		pslotinfo->dma_w_fill_counter = 0;
		pslotinfo->dma_w_counter = 0;
		pslotinfo->dma_r_fill_counter = 0;
		pslotinfo->dma_r_counter = 0;
#endif
	} else {

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
		struct ipc_sdinfo *sdinfo = ambarella_sd_get_sdinfo(mmc);
		if (sdinfo->is_init && sdinfo->from_ipc)
			goto done;
#endif

		desired_clk = clk;
		if (desired_clk > pinfo->pcontroller->clk_limit)
			desired_clk = pinfo->pcontroller->clk_limit;

		if (pinfo->pcontroller->support_pll_scaler) {
			if (desired_clk < 10000000) {
				/* Below 10Mhz, divide by sd controller */
				pinfo->pcontroller->set_pll(
					pinfo->pcontroller->clk_limit);
			} else {
				pinfo->pcontroller->set_pll(desired_clk);
				actual_clk = pinfo->pcontroller->get_pll();
				bneed_div = 0;
			}
		} else {
			pinfo->pcontroller->set_pll(48000000);
		}

		if (bneed_div) {
			sd_clk = pinfo->pcontroller->get_pll();
			for (clk_div = 0x0; clk_div <= 0x80;) {
				if (clk_div == 0)
					actual_clk = sd_clk;
				else
					actual_clk = sd_clk / (clk_div << 1);

				if (actual_clk <= desired_clk)
					break;

				if (clk_div >= 0x80)
					break;

				if (clk_div == 0x0)
					clk_div = 0x1;
				else
					clk_div <<= 1;
			}
			ambsd_dbg(pslotinfo, "sd_clk = %d.\n", sd_clk);
		}
		ambsd_dbg(pslotinfo, "desired_clk = %d.\n", desired_clk);
		ambsd_dbg(pslotinfo, "actual_clk = %d.\n", actual_clk);
		ambsd_dbg(pslotinfo, "clk_div = %d.\n", clk_div);

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
done:
#endif
		clk_div <<= 8;
		clk_div |= SD_CLK_ICLK_EN;
		amba_writew(pinfo->regbase + SD_CLK_OFFSET, clk_div);
		while (1) {
			clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
			if (clkreg & SD_CLK_ICLK_STABLE)
				break;
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				ambsd_warn(pslotinfo,
					"Wait SD_CLK_ICLK_STABLE...\n");
				break;
			}
		}
		clkreg |= SD_CLK_EN;
		amba_writew(pinfo->regbase + SD_CLK_OFFSET, clkreg);
	}
}

static void ambarella_sd_set_pwr(struct mmc_host *mmc, u32 pwr_mode, u32 vdd)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	u8					pwr = SD_PWR_OFF;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (pwr_mode == MMC_POWER_OFF) {
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET, pwr);
		ambarella_set_gpio_output(&pslotinfo->plat_info->ext_reset, 1);
		ambarella_set_gpio_output(&pslotinfo->plat_info->ext_power, 0);
	} else if (pwr_mode == MMC_POWER_UP) {
		ambarella_set_gpio_output(&pslotinfo->plat_info->ext_power, 1);
		ambarella_set_gpio_output(&pslotinfo->plat_info->ext_reset, 0);
	}

	if ((pwr_mode == MMC_POWER_ON) || (pwr_mode == MMC_POWER_UP)) {
		pwr = SD_PWR_ON;
		switch (1 << vdd) {
		case MMC_VDD_165_195:
			pwr |= SD_PWR_1_8V;
			break;
		case MMC_VDD_29_30:
		case MMC_VDD_30_31:
			pwr |= SD_PWR_3_0V;
			break;
		case MMC_VDD_32_33:
		case MMC_VDD_33_34:
			pwr |= SD_PWR_3_3V;
			break;
		default:
			pwr = SD_PWR_OFF;
			ambsd_err(pslotinfo, "%s Wrong voltage[%d]!\n",
				__func__, vdd);
			break;
		}
		if (amba_readb(pinfo->regbase + SD_PWR_OFFSET) != pwr) {
			amba_writeb(pinfo->regbase + SD_PWR_OFFSET, pwr);
			mdelay(pslotinfo->plat_info->ext_power.active_delay);
		}
	}

	ambsd_dbg(pslotinfo, "pwr = 0x%x.\n", pwr);
}

static void ambarella_sd_set_bus(struct mmc_host *mmc,
	u32 bus_width, u32 timing)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	u8					hostr = 0;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	hostr = amba_readb(pinfo->regbase + SD_HOST_OFFSET);
	if (bus_width == MMC_BUS_WIDTH_8) {
		hostr |= SD_HOST_8BIT;
		hostr &= ~(SD_HOST_4BIT);
	} else if (bus_width == MMC_BUS_WIDTH_4) {
		hostr &= ~(SD_HOST_8BIT);
		hostr |= SD_HOST_4BIT;
	} else if (bus_width == MMC_BUS_WIDTH_1) {
		hostr &= ~(SD_HOST_8BIT);
		hostr &= ~(SD_HOST_4BIT);
	}
#if 0
	if ((timing == MMC_TIMING_SD_HS) || (timing == MMC_TIMING_MMC_HS)) {
		ambsd_warn(pslotinfo, "Switch to high speed mode!\n");
		hostr |= SD_HOST_HIGH_SPEED;
	} else {
		hostr &= ~SD_HOST_HIGH_SPEED;
	}
#else
	hostr &= ~SD_HOST_HIGH_SPEED;
#endif
	amba_writeb(pinfo->regbase + SD_HOST_OFFSET, hostr);

	ambsd_dbg(pslotinfo, "hostr = 0x%x.\n", hostr);
}

static void ambarella_sd_check_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	struct ipc_sdinfo *sdinfo = ambarella_sd_get_sdinfo(mmc);
	if (sdinfo->from_ipc && !sdinfo->is_sdio) {
		ios->bus_width = (sdinfo->bus_width == 8) ? MMC_BUS_WIDTH_8:
				 (sdinfo->bus_width == 4) ? MMC_BUS_WIDTH_4:
							    MMC_BUS_WIDTH_1;
		ios->clock = sdinfo->clk;
	}
#endif
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if ((pinfo->controller_ios.power_mode != ios->power_mode) ||
		(pinfo->controller_ios.vdd != ios->vdd) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_pwr(mmc, ios->power_mode, ios->vdd);
		pinfo->controller_ios.power_mode = ios->power_mode;
		pinfo->controller_ios.vdd = ios->vdd;
	}

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	if ((amba_readw(pinfo->regbase + SD_CLK_OFFSET) & SD_CLK_EN) == 0) {
		/* uitron will on/off clock every sd request, */
		/* if clock is diable, that means uitron has ever access sd */
		/* controller and we need to set to correct clock again. */
		pinfo->controller_ios.clock = 0;
	}
#endif

	if ((pinfo->controller_ios.clock != ios->clock) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		mdelay(10);
		ambarella_sd_set_clk(mmc, ios->clock);
		pinfo->controller_ios.clock = ios->clock;
		mdelay(10);
	}

	if ((pinfo->controller_ios.bus_width != ios->bus_width) ||
		(pinfo->controller_ios.timing != ios->timing) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_bus(mmc, ios->bus_width, ios->timing);
		pinfo->controller_ios.bus_width = ios->bus_width;
		pinfo->controller_ios.timing = ios->timing;
	}

	pslotinfo->state = AMBA_SD_STATE_IDLE;
}

static void ambarella_sd_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	ambarella_sd_request_bus(mmc);
	ambarella_sd_check_ios(mmc, ios);
	ambarella_sd_release_bus(mmc);
}

static int ambarella_sd_get_ro(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	u32					wpspl;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_request_bus(mmc);

	if (pslotinfo->plat_info->fixed_wp == -1) {
		if (pslotinfo->plat_info->gpio_wp.gpio_id != -1) {
			wpspl = !ambarella_get_gpio_input(
				&pslotinfo->plat_info->gpio_wp);
		} else {
			wpspl = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			ambsd_dbg(pslotinfo, "SD/MMC RD[0x%x].\n", wpspl);
			wpspl &= SD_STA_WPS_PL;
		}
	} else {
		wpspl =	pslotinfo->plat_info->fixed_wp;
	}

	ambarella_sd_release_bus(mmc);

	return wpspl ? 1 : 0;
}

static void ambarella_sd_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	long					timeout;
	u32					card_sta;
	u32					need_reset = 0;
	struct ambarella_sd_controller_info	*pinfo;
	u32					valid_request = 0;
	u32					valid_cd = 0;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_request_bus(mmc);
#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	/* ambarella_sd_vic_ctrl(pinfo->irq - 32, 0); */
	boss_set_irq_owner(pinfo->irq, BOSS_IRQ_OWNER_LINUX, 1);
#endif

	pslotinfo->mrq = mrq;

	card_sta = amba_readl(pinfo->regbase + SD_STA_OFFSET);

	if (pslotinfo->plat_info->fixed_cd == 1) {
		valid_request = 1;
	} else if (pslotinfo->plat_info->fixed_cd == 0) {
		valid_request = 0;
	} else {
		valid_cd = ambarella_sd_gpio_cd_check_val(pslotinfo);
		if (valid_cd == 1) {
			valid_request = 1;
		} else if (valid_cd == -1) {
			if (card_sta & SD_STA_CARD_INSERTED) {
				valid_request = 1;
			}
		}
	}

	ambsd_dbg(pslotinfo, "0x%08x:%d & cmd = %d valid_request = %d.\n",
		card_sta, valid_cd, mrq->cmd->opcode, valid_request);
	if (valid_request) {
		ambarella_sd_check_ios(mmc, &mmc->ios);
		ambarella_sd_send_cmd(pslotinfo, mrq->cmd, mrq->stop);
		if (pslotinfo->state != AMBA_SD_STATE_ERR) {
			timeout = wait_event_timeout(pslotinfo->wait,
				(pslotinfo->state == AMBA_SD_STATE_IDLE),
				pinfo->pcontroller->wait_tmo);
			if (timeout <= 0) {
				ambsd_err(pslotinfo, "%s cmd%d timeout "
					"%d@%d, retries=%d\n",
					__func__, mrq->cmd->opcode,
					pslotinfo->state,
					pinfo->pcontroller->wait_tmo,
					mrq->cmd->retries);
				goto ambarella_sd_request_need_reset;
			}
			ambsd_dbg(pslotinfo, "%ld jiffies left.\n", timeout);
		} else {
			need_reset = 1;
			goto ambarella_sd_request_need_reset;
		}
	} else {
		mrq->cmd->error = -ENOMEDIUM;
		goto ambarella_sd_request_exit;
	}

ambarella_sd_request_need_reset:
	if (pslotinfo->state != AMBA_SD_STATE_IDLE)
		need_reset = 1;

	if ((mrq->stop) && (mrq->stop->error)) {
		ambsd_dbg(pslotinfo, "stop error = %d\n", mrq->stop->error);
		need_reset = 1;
	}

	if ((mrq->cmd->data) && (mrq->cmd->data->error)) {
		ambsd_dbg(pslotinfo, "data error = %d\n",
			mrq->cmd->data->error);
		need_reset = 1;
	}

	if (mrq->cmd->error) {
		ambsd_dbg(pslotinfo, "cmd error = %d\n", mrq->cmd->error);
		need_reset = 1;
	}

	if (need_reset) {
		ambsd_dbg(pslotinfo, "need_reset %d %d 0x%x!\n",
			pslotinfo->state, mrq->cmd->opcode, card_sta);
		ambarella_sd_reset_all(pslotinfo->mmc);
	}

ambarella_sd_request_exit:
	pslotinfo->mrq = NULL;

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	/* ambarella_sd_vic_ctrl(pinfo->irq - 32, 1); */
#endif
	ambarella_sd_release_bus(mmc);

	mmc_request_done(mmc, mrq);
}

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
static int ambarella_sd_card_in_slot(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	u32					valid_cd;

	if (pslotinfo->plat_info->fixed_cd == 1) {
		valid_cd = 1;
	} else if (pslotinfo->plat_info->fixed_cd == 0) {
		valid_cd = 0;
	} else {
		valid_cd = ambarella_sd_gpio_cd_check_val(pslotinfo);
		if (valid_cd < 0)
			valid_cd = 0;
	}

	return valid_cd;
}
#endif

#ifdef CONFIG_TIWLAN_SDIO
static int ambarella_sd_get_cd(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *host = mmc_priv(mmc);

	if (!host->plat_info->card_detect)
		return -ENOSYS;
	return host->plat_info->card_detect(NULL, host->slot_id);
}
#endif

static const struct mmc_host_ops ambarella_sd_host_ops = {
	.request	= ambarella_sd_request,
	.set_ios	= ambarella_sd_ios,
	.get_ro		= ambarella_sd_get_ro,
	.enable_sdio_irq= ambarella_sd_enable_sdio_irq,
#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	.get_cd		= ambarella_sd_card_in_slot,
#endif

#ifdef CONFIG_TIWLAN_SDIO
	.get_cd = ambarella_sd_get_cd,
#endif
};

static int ambarella_sd_system_event(struct notifier_block *nb,
	unsigned long val, void *data)
{
	int					errorCode = NOTIFY_OK;
	struct ambarella_sd_mmc_info		*pslotinfo;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = container_of(nb, struct ambarella_sd_mmc_info,
		system_event);
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	switch (val) {
	case AMBA_EVENT_PRE_CPUFREQ:
		pr_debug("%s[%d]: Pre Change\n", __func__, pslotinfo->slot_id);
		down(&pslotinfo->system_event_sem);
		break;

	case AMBA_EVENT_POST_CPUFREQ:
		pr_debug("%s[%d]: Post Change\n", __func__, pslotinfo->slot_id);
		msleep(10);
		ambarella_sd_set_clk(pslotinfo->mmc,
			pinfo->controller_ios.clock);
		msleep(10);
		up(&pslotinfo->system_event_sem);
		break;

	default:
		break;
	}

	return errorCode;
}
#ifdef CONFIG_TIWLAN_SDIO
static void ambarella_hsmmc_status_notify_cb(int card_present, void *dev_id)
{

       struct ambarella_sd_mmc_info *device = dev_id;
       struct ambarella_sd_slot *slot = device->plat_info;
       int carddetect;

       printk(KERN_NOTICE "%s %s : card_present %d\n", mmc_hostname(slot->pmmc_host),__func__,card_present);

       carddetect = slot->card_detect(0, 0);

//       sysfs_notify(&host->mmc->class_dev.kobj, NULL, "cover_switch");
#if 0
	if (carddetect)
		mmc_detect_change(slot->pmmc_host, (HZ * 200) / 1000);
	else
		mmc_detect_change(slot->pmmc_host, (HZ * 50) / 1000);
#endif
	if (carddetect)
		mmc_detect_change(slot->pmmc_host, slot->cd_delay);
	else
		mmc_detect_change(slot->pmmc_host, slot->cd_delay);

}
#endif

/* ==========================================================================*/
static int __devinit ambarella_sd_probe(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_sd_controller_info	*pinfo = NULL;
	struct ambarella_sd_mmc_info		*pslotinfo = NULL;
	struct resource 			*irq;
	struct resource 			*mem;
	struct resource 			*ioarea;
	struct mmc_host				*mmc = NULL;
	u32					hc_cap = 0;
	u32					i;
	u32					clock_min;
#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	struct ipc_sdinfo 			*sdinfo;
#endif
#ifdef CONFIG_TIWLAN_SDIO
struct ambarella_sd_slot * tristan_plat_info;
#endif


	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get SD/MMC mem resource failed!\n");
		errorCode = -ENXIO;
		goto sd_errorCode_na;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq == NULL) {
		dev_err(&pdev->dev, "Get SD/MMC irq resource failed!\n");
		errorCode = -ENXIO;
		goto sd_errorCode_na;
	}

	ioarea = request_mem_region(mem->start,
			(mem->end - mem->start) + 1, pdev->name);
	if (ioarea == NULL) {
		dev_err(&pdev->dev, "Request SD/MMC ioarea failed!\n");
		errorCode = -EBUSY;
		goto sd_errorCode_na;
	}

	pinfo = kzalloc(sizeof(struct ambarella_sd_controller_info),
		GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		errorCode = -ENOMEM;
		goto sd_errorCode_ioarea;
	}
#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	pinfo->id = pdev->id;
#endif
	pinfo->regbase = (unsigned char __iomem *)mem->start;
	pinfo->dev = &pdev->dev;
	pinfo->mem = mem;
	pinfo->irq = irq->start;
	spin_lock_init(&pinfo->lock);
	pinfo->pcontroller =
		(struct ambarella_sd_controller *)pdev->dev.platform_data;
	if ((pinfo->pcontroller == NULL) ||
		(pinfo->pcontroller->get_pll == NULL) ||
		(pinfo->pcontroller->set_pll == NULL)) {
		dev_err(&pdev->dev, "Need SD/MMC controller info!\n");
		errorCode = -EPERM;
		goto sd_errorCode_free_pinfo;
	}

	if (pinfo->pcontroller->wait_tmo <
		CONFIG_SD_AMBARELLA_WAIT_TIMEOUT) {
		dev_dbg(&pdev->dev, "Change wait timeout from %d to %d.\n",
			pinfo->pcontroller->wait_tmo,
			CONFIG_SD_AMBARELLA_WAIT_TIMEOUT);
		pinfo->pcontroller->wait_tmo =
			CONFIG_SD_AMBARELLA_WAIT_TIMEOUT;
	}

	pinfo->pcontroller->set_pll(pinfo->pcontroller->max_clk);
	clock_min = pinfo->pcontroller->get_pll() >> 8;
	if (pinfo->pcontroller->clk_limit < clock_min) {
		dev_dbg(&pdev->dev,
			"Change clock limit from %dKHz to %dKHz.\n",
			pinfo->pcontroller->clk_limit,
			clock_min);
		pinfo->pcontroller->clk_limit = clock_min;
	}

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		mmc = mmc_alloc_host(sizeof(struct ambarella_sd_mmc_info),
			&pdev->dev);
		if (!mmc) {
			dev_err(&pdev->dev, "Failed to allocate mmc host"
				" for slot%d!\n", i);
			errorCode = -ENOMEM;
			goto sd_errorCode_free_host;
		}
		mmc->ops = &ambarella_sd_host_ops;

		pinfo->pslotinfo[i] = pslotinfo = mmc_priv(mmc);
		pslotinfo->mmc = mmc;
		init_waitqueue_head(&pslotinfo->wait);
		pslotinfo->state = AMBA_SD_STATE_ERR;
		pslotinfo->dma_w_fill_counter = 0;
		pslotinfo->dma_w_counter = 0;
		pslotinfo->dma_r_fill_counter = 0;
		pslotinfo->dma_r_counter = 0;
		pslotinfo->plat_info = &(pinfo->pcontroller->slot[i]);
		pslotinfo->slot_id = i;
		pslotinfo->nisen = 0;
		pslotinfo->pinfo = pinfo;
		sema_init(&pslotinfo->system_event_sem, 1);

		ambarella_sd_request_bus(mmc);

		ambarella_sd_reset_all(mmc);

		hc_cap = amba_readl(pinfo->regbase + SD_CAP_OFFSET);
		if (hc_cap & SD_CAP_TOCLK_MHZ)
			dev_dbg(&pdev->dev,
				"Timeout Clock Frequency: %dMHz.\n",
				SD_CAP_TOCLK_FREQ(hc_cap));
		else
			dev_dbg(&pdev->dev,
				"Timeout Clock Frequency: %dKHz.\n",
				SD_CAP_TOCLK_FREQ(hc_cap));

		mmc->f_min = clock_min;
		mmc->f_max = pinfo->pcontroller->max_clk;
		dev_dbg(&pdev->dev,
			"SD Clock: base[%dMHz], min[%dHz], max[%dHz].\n",
			SD_CAP_BASE_FREQ(hc_cap),
			mmc->f_min,
			mmc->f_max);

		if (hc_cap & SD_CAP_MAX_2KB_BLK)
			mmc->max_blk_size = 2048;
		else if (hc_cap & SD_CAP_MAX_1KB_BLK)
			mmc->max_blk_size = 1024;
		else if (hc_cap & SD_CAP_MAX_512B_BLK)
			mmc->max_blk_size = 512;
		dev_dbg(&pdev->dev,
			"SD max_blk_size: %d.\n",
			mmc->max_blk_size);

		mmc->caps = pslotinfo->plat_info->caps;
		if (hc_cap & SD_CAP_HIGH_SPEED) {
			mmc->caps |= MMC_CAP_SD_HIGHSPEED;
			mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		}
		dev_dbg(&pdev->dev, "SD caps: 0x%lx.\n", mmc->caps);

		if (hc_cap & SD_CAP_DMA) {
			dev_dbg(&pdev->dev, "HW support DMA!\n");
		} else {
			ambsd_err(pslotinfo, "HW do not support DMA!\n");
			errorCode = -ENODEV;
			goto sd_errorCode_free_host;
		}

		if (hc_cap & SD_CAP_SUS_RES)
			dev_dbg(&pdev->dev, "HW support Suspend/Resume!\n");
		else
			dev_dbg(&pdev->dev,
				"HW do not support Suspend/Resume!\n");

		mmc->ocr_avail = 0;
		if (hc_cap & SD_CAP_VOL_1_8V)
			mmc->ocr_avail |= MMC_VDD_165_195;
		if (hc_cap & SD_CAP_VOL_3_0V)
			mmc->ocr_avail |= MMC_VDD_29_30 | MMC_VDD_30_31;
		if (hc_cap & SD_CAP_VOL_3_3V)
			mmc->ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34;
		if (mmc->ocr_avail == 0) {
			ambsd_err(pslotinfo,
				"Hardware report wrong voltages[0x%x].!\n",
				hc_cap);
			errorCode = -ENODEV;
			goto sd_errorCode_free_host;
		}

		if (hc_cap & SD_CAP_INTMODE) {
			dev_dbg(&pdev->dev, "HW support Interrupt mode!\n");
		} else {
			ambsd_err(pslotinfo,
				"HW do not support Interrupt mode!\n");
			errorCode = -ENODEV;
			goto sd_errorCode_free_host;
		}

		pslotinfo->max_blk_sz = pslotinfo->plat_info->max_blk_sz;
		mmc->max_seg_size = ambarella_sd_dma_mask_to_size(
			pslotinfo->max_blk_sz);
		mmc->max_blk_count = 0xFFFF;
		mmc->max_req_size = min(mmc->max_blk_size * mmc->max_blk_count,
			mmc->max_seg_size);
		if (pslotinfo->plat_info->use_bounce_buffer) {
			mmc->max_segs = 128;
			pslotinfo->max_blk_sz =
				pslotinfo->plat_info->max_blk_sz;
			mmc->max_seg_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->max_blk_sz);
			mmc->max_blk_count = 0xFFFF;
			mmc->max_req_size = min(mmc->max_seg_size,
				mmc->max_blk_size * mmc->max_blk_count);
			pslotinfo->buf_vaddress = kmalloc(
				mmc->max_req_size, GFP_KERNEL);
			if (!pslotinfo->buf_vaddress) {
				ambsd_err(pslotinfo, "Can't alloc DMA memory");
				errorCode = -ENOMEM;
				goto sd_errorCode_free_host;
			}
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
			pslotinfo->buf_paddress = dma_map_single(
				pinfo->dev, pslotinfo->buf_vaddress,
				mmc->max_req_size, DMA_BIDIRECTIONAL);
#else
			pslotinfo->buf_paddress = (dma_addr_t)__virt_to_phys(
				pslotinfo->buf_vaddress);
#endif
			if (ambarella_sd_check_dma_boundary(pslotinfo,
				pslotinfo->buf_paddress, mmc->max_req_size,
				ambarella_sd_dma_mask_to_size(
				pslotinfo->max_blk_sz)) == 0) {
				ambsd_err(pslotinfo, "DMA boundary err!\n");
				errorCode = -ENOMEM;
				goto sd_errorCode_free_host;
			}
			dev_notice(&pdev->dev, "Slot%d bounce buffer:"
				"0x%p<->0x%08x, size=%d, req_size=%d\n",
				pslotinfo->slot_id, pslotinfo->buf_vaddress,
				pslotinfo->buf_paddress, mmc->max_seg_size,
				mmc->max_req_size);
		} else {
			mmc->max_segs = 1;
			pslotinfo->max_blk_sz = SD_BLK_SZ_512KB;
			mmc->max_seg_size = 0x40000;
			mmc->max_blk_count = 0xFFFF;
			mmc->max_req_size = min(mmc->max_seg_size,
				mmc->max_blk_size * mmc->max_blk_count);
			pslotinfo->buf_paddress = (dma_addr_t)NULL;
			pslotinfo->buf_vaddress = NULL;
		}

		if (pslotinfo->plat_info->ext_power.gpio_id != -1) {
			errorCode = gpio_request(
				pslotinfo->plat_info->ext_power.gpio_id,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get Power GPIO%d\n",
				pslotinfo->plat_info->ext_power.gpio_id);
				goto sd_errorCode_free_host;
			}
		}

		if (pslotinfo->plat_info->ext_reset.gpio_id != -1) {
			errorCode = gpio_request(
				pslotinfo->plat_info->ext_reset.gpio_id,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get Reset GPIO%d\n",
				pslotinfo->plat_info->ext_reset.gpio_id);
				goto sd_errorCode_free_host;
			}
		}

		if (pslotinfo->plat_info->gpio_wp.gpio_id != -1) {
			errorCode = gpio_request(
				pslotinfo->plat_info->gpio_wp.gpio_id,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get WP GPIO%d\n",
				pslotinfo->plat_info->gpio_wp.gpio_id);
				goto sd_errorCode_free_host;
			}
		}

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
		ambarella_sdinfo_ipc(mmc);
		sdinfo = ambarella_sd_get_sdinfo(mmc);
		ambarella_sd_cmd_from_ipc(mmc, sdinfo->is_init);
		/* Set clock back to Itron desired. */
		if (sdinfo->is_init)
			pinfo->pcontroller->set_pll(sdinfo->clk);

		mmc->pm_flags |= MMC_PM_KEEP_POWER;
#endif

		ambarella_sd_release_bus(mmc);
	}

	errorCode = request_irq(pinfo->irq,
		ambarella_sd_irq,
		IRQF_SHARED | IRQF_TRIGGER_HIGH,
		dev_name(&pdev->dev), pinfo);
	if (errorCode) {
		dev_err(&pdev->dev, "Can't Request IRQ%d!\n", pinfo->irq);
		goto sd_errorCode_free_host;
	}

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	disable_irq(pinfo->irq);
#endif
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		pslotinfo->system_event.notifier_call =
			ambarella_sd_system_event;
		ambarella_register_event_notifier(&pslotinfo->system_event);

		if (ambarella_is_valid_gpio_irq(
			&pslotinfo->plat_info->gpio_cd)) {
			errorCode = gpio_request(
				pslotinfo->plat_info->gpio_cd.irq_gpio,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get CD GPIO%d\n",
				pslotinfo->plat_info->gpio_cd.irq_gpio);
				goto sd_errorCode_free_host;
			}
#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
			sdinfo = ambarella_sd_get_sdinfo(mmc);
			if (sdinfo->is_init)
				continue;
#endif
			ambarella_sd_gpio_cd_check_val(pslotinfo);
			errorCode = request_irq(
				pslotinfo->plat_info->gpio_cd.irq_line,
				ambarella_sd_gpio_cd_irq,
				pslotinfo->plat_info->gpio_cd.irq_type,
				dev_name(&pdev->dev), pslotinfo);
			if (errorCode) {
				ambsd_err(pslotinfo,
				"Can't Request GPIO(%d) CD IRQ(%d)!\n",
				pslotinfo->plat_info->gpio_cd.irq_gpio,
				pslotinfo->plat_info->gpio_cd.irq_line);
				goto sd_errorCode_free_host;
			}
		}
	}

	platform_set_drvdata(pdev, pinfo);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		pslotinfo->plat_info->pmmc_host = pslotinfo->mmc;
		pslotinfo->valid = 1;
		errorCode = mmc_add_host(pslotinfo->mmc);
		if (errorCode) {
			ambsd_err(pslotinfo, "Can't add mmc host!\n");
			goto sd_errorCode_remove_host;
		}
#ifdef CONFIG_TIWLAN_SDIO

		if(pdev->id == 0 && i==0){//controller =0 && slot=0
		tristan_plat_info = &(pinfo->pcontroller->slot[i]);
		mmc_set_embedded_sdio_data(pslotinfo->plat_info->pmmc_host,
		&tristan_plat_info->embedded_sdio->cis,
		&tristan_plat_info->embedded_sdio->cccr,
		tristan_plat_info->embedded_sdio->funcs,
		tristan_plat_info->embedded_sdio->quirks);
		if (pslotinfo->plat_info->register_status_notify)
			{
			printk(KERN_NOTICE "%s: register notifyCB for notify function \n",
				mmc_hostname(pinfo->pslotinfo[i]->mmc));
				pinfo->pslotinfo[i]->plat_info->register_status_notify(ambarella_hsmmc_status_notify_cb, pslotinfo);
			}

		}
#endif

	}

	dev_notice(&pdev->dev,
		"Ambarella SD/MMC[%d] probed %d slots, 0x%08x!\n",
		pdev->id, pinfo->pcontroller->num_slots, hc_cap);

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	enable_irq(pinfo->irq);
#endif
	goto sd_errorCode_na;

sd_errorCode_remove_host:
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		pslotinfo->plat_info->pmmc_host = NULL;
		if ((pslotinfo->mmc) && (pslotinfo->valid == 1))
			mmc_remove_host(pslotinfo->mmc);
	}

	free_irq(pinfo->irq, pinfo);

sd_errorCode_free_host:
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd)) {
			free_irq(pslotinfo->plat_info->gpio_cd.irq_line, pslotinfo);
			gpio_free(pslotinfo->plat_info->gpio_cd.irq_gpio);
		}
		if (pslotinfo->plat_info->gpio_wp.gpio_id != -1)
			gpio_free(pslotinfo->plat_info->gpio_wp.gpio_id);
		if (pslotinfo->plat_info->ext_power.gpio_id != -1)
			gpio_free(pslotinfo->plat_info->ext_power.gpio_id);
		if (pslotinfo->plat_info->ext_reset.gpio_id != -1)
			gpio_free(pslotinfo->plat_info->ext_reset.gpio_id);
		if (pslotinfo->buf_paddress) {
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
			dma_unmap_single(pinfo->dev, pslotinfo->buf_paddress,
				pslotinfo->mmc->max_req_size, DMA_FROM_DEVICE);
#endif
			pslotinfo->buf_paddress = (dma_addr_t)NULL;
		}
		if (pslotinfo->buf_vaddress) {
			kfree(pslotinfo->buf_vaddress);
			pslotinfo->buf_vaddress = NULL;
			pslotinfo->buf_paddress = (dma_addr_t)NULL;
		}
		if (pslotinfo->mmc);
			mmc_free_host(pslotinfo->mmc);
	}

sd_errorCode_free_pinfo:
	kfree(pinfo);

sd_errorCode_ioarea:
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

sd_errorCode_na:
	return errorCode;
}

static int __devexit ambarella_sd_remove(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		platform_set_drvdata(pdev, NULL);

		free_irq(pinfo->irq, pinfo);

		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			pslotinfo->plat_info->pmmc_host = NULL;
			ambarella_unregister_event_notifier(&pslotinfo->system_event);
			if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd)) {
				free_irq(pslotinfo->plat_info->gpio_cd.irq_line, pslotinfo);
				gpio_free(pslotinfo->plat_info->gpio_cd.irq_gpio);
			}
			if (pslotinfo->mmc)
				mmc_remove_host(pslotinfo->mmc);
		}

		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			if (pslotinfo->plat_info->ext_power.gpio_id != -1)
				gpio_free(pslotinfo->plat_info->ext_power.gpio_id);
			if (pslotinfo->plat_info->ext_reset.gpio_id != -1)
				gpio_free(pslotinfo->plat_info->ext_reset.gpio_id);
			if (pslotinfo->plat_info->gpio_wp.gpio_id != -1)
				gpio_free(pslotinfo->plat_info->gpio_wp.gpio_id);
			if (pslotinfo->buf_paddress) {
#ifdef CONFIG_SD_AMBARELLA_SYNC_DMA_STANDARD
				dma_unmap_single(pinfo->dev,
					pslotinfo->buf_paddress,
					pslotinfo->mmc->max_req_size,
					DMA_FROM_DEVICE);
#endif
				pslotinfo->buf_paddress = (dma_addr_t)NULL;
			}
			if (pslotinfo->buf_vaddress) {
				kfree(pslotinfo->buf_vaddress);
				pslotinfo->buf_vaddress = NULL;
			}
			if (pslotinfo->mmc)
				mmc_free_host(pslotinfo->mmc);
		}
		release_mem_region(pinfo->mem->start,
			(pinfo->mem->end - pinfo->mem->start) + 1);
		kfree(pinfo);
	}

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor SD/MMC Host Controller.\n");

	return errorCode;
}

#ifdef CONFIG_PM
static int ambarella_sd_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int					errorCode = 0;
#if !defined(CONFIG_AMBARELLA_IPC) || defined(CONFIG_BOSS_SINGLE_CORE)
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;

	pinfo = platform_get_drvdata(pdev);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			pslotinfo->state = AMBA_SD_STATE_RESET;
			errorCode = mmc_suspend_host(pslotinfo->mmc);
			if (errorCode) {
				ambsd_err(pslotinfo,
				"mmc_suspend_host[%d] failed[%d]!\n",
				i, errorCode);
			}
		}
	}

	disable_irq(pinfo->irq);
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd))
			disable_irq(pslotinfo->plat_info->gpio_cd.irq_line);
	}

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);
#endif
	return errorCode;
}

static int ambarella_sd_resume(struct platform_device *pdev)
{
	int					errorCode = 0;
#if !defined(CONFIG_AMBARELLA_IPC) || defined(CONFIG_BOSS_SINGLE_CORE)
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;

	pinfo = platform_get_drvdata(pdev);
	pinfo->pcontroller->set_pll(pinfo->pcontroller->max_clk);
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		ambarella_sd_reset_all(pslotinfo->mmc);
		if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd))
			enable_irq(pslotinfo->plat_info->gpio_cd.irq_line);
	}
	enable_irq(pinfo->irq);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			errorCode = mmc_resume_host(pslotinfo->mmc);
			if (errorCode) {
				ambsd_err(pslotinfo,
				"mmc_resume_host[%d] failed[%d]!\n",
				i, errorCode);
			}
		}
	}

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);
#endif
	return errorCode;
}
#endif

static struct platform_driver ambarella_sd_driver = {
	.probe		= ambarella_sd_probe,
	.remove		= __devexit_p(ambarella_sd_remove),
#ifdef CONFIG_PM
	.suspend	= ambarella_sd_suspend,
	.resume		= ambarella_sd_resume,
#endif
	.driver		= {
		.name	= "ambarella-sd",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_sd_init(void)
{
	int				errorCode = 0;

#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)
	memset(G_ipc_sdinfo, 0x0, sizeof(G_ipc_sdinfo));
#endif

	errorCode = platform_driver_register(&ambarella_sd_driver);
	if (errorCode) {
		printk(KERN_ERR "%s: Register failed %d!\n",
		__func__, errorCode);
	}

	return errorCode;
}

static void __exit ambarella_sd_exit(void)
{
	platform_driver_unregister(&ambarella_sd_driver);
}


#if defined(CONFIG_AMBARELLA_IPC) && !defined(CONFIG_NOT_SHARE_SD_CONTROLLER_WITH_UITRON)

/**
 * ambarella_sdmmc_cmd_ipc.
 */
int ambarella_sdmmc_cmd_ipc(struct mmc_host *mmc, struct mmc_command *cmd)
{
	struct ipc_sdresp 			resp;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	resp.opcode = cmd->opcode;
	resp.slot_id = (pinfo->id == 0 && pslotinfo->slot_id == 0) ? SD_HOST1_SD:
		       (pinfo->id == 0 && pslotinfo->slot_id == 1) ? SD_HOST1_SDIO:
		       						     SD_HOST2_SD;
	if (G_ipc_sdinfo[resp.slot_id].from_ipc == 0)
		return -1;

	/* send IPC */
	ipc_sdresp_get(0, &resp);
	if (resp.ret != 0)
		return resp.ret;

	memcpy(cmd->resp, resp.resp, sizeof(u32) * 4);

	if (cmd->data != NULL) {
		memcpy(cmd->data->buf, resp.buf, cmd->data->blksz);
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_sdmmc_cmd_ipc);

struct ipc_sdinfo *ambarella_sd_get_sdinfo(struct mmc_host *mmc)
{
	u32 slot_id;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	BUG_ON(!mmc);

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
	slot_id = (pinfo->id == 0 && pslotinfo->slot_id == 0) ? SD_HOST1_SD:
		  (pinfo->id == 0 && pslotinfo->slot_id == 1) ? SD_HOST1_SDIO:
								SD_HOST2_SD;
	return &G_ipc_sdinfo[slot_id];
}

EXPORT_SYMBOL(ambarella_sd_get_sdinfo);

/**
 * Service initialization.
 */
int ambarella_sdinfo_ipc(struct mmc_host *mmc)
{
	u32 slot_id;
	struct ipc_sdinfo ipc_sdinfo;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
	slot_id = (pinfo->id == 0 && pslotinfo->slot_id == 0) ? SD_HOST1_SD:
		  (pinfo->id == 0 && pslotinfo->slot_id == 1) ? SD_HOST1_SDIO:
								SD_HOST2_SD;
	memset(&ipc_sdinfo, 0x0, sizeof(ipc_sdinfo));
	ipc_sdinfo.slot_id = slot_id;
	if (ipc_sdinfo_get(0, &ipc_sdinfo) < 0) {
		memset(&G_ipc_sdinfo[slot_id], 0x0, sizeof(ipc_sdinfo));
		return -1;
	}

	G_ipc_sdinfo[slot_id].is_init 	= ipc_sdinfo.is_init;
	G_ipc_sdinfo[slot_id].is_sdmem 	= ipc_sdinfo.is_sdmem;
	G_ipc_sdinfo[slot_id].is_mmc 	= ipc_sdinfo.is_mmc;
	G_ipc_sdinfo[slot_id].is_sdio 	= ipc_sdinfo.is_sdio;
	G_ipc_sdinfo[slot_id].bus_width	= ipc_sdinfo.bus_width;
	G_ipc_sdinfo[slot_id].clk 	= ipc_sdinfo.clk;
	G_ipc_sdinfo[slot_id].ocr 	= ipc_sdinfo.ocr;
	G_ipc_sdinfo[slot_id].hcs 	= ipc_sdinfo.hcs;
	G_ipc_sdinfo[slot_id].rca 	= ipc_sdinfo.rca;

	G_mmc[slot_id] = mmc;

#if 0
	printk("%s: ipc_sdinfo.is_init = %d\n", __func__, ipc_sdinfo.is_init);
	printk("%s: ipc_sdinfo.is_sdmem = %d\n", __func__, ipc_sdinfo.is_sdmem);
	printk("%s: ipc_sdinfo.is_mmc = %d\n", __func__, ipc_sdinfo.is_mmc);
	printk("%s: ipc_sdinfo.is_sdio = %d\n", __func__, ipc_sdinfo.is_sdio);
	printk("%s: ipc_sdinfo.bus_width = %d\n", __func__, ipc_sdinfo.bus_width);
#endif
	return 0;
}

EXPORT_SYMBOL(ambarella_sdinfo_ipc);

/**
 * ambarella_sd_cmd_from_ipc.
 */
void ambarella_sd_cmd_from_ipc(struct mmc_host *mmc, u8 enable)
{
	u32 slot_id;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
	slot_id = (pinfo->id == 0 && pslotinfo->slot_id == 0) ? SD_HOST1_SD:
		  (pinfo->id == 0 && pslotinfo->slot_id == 1) ? SD_HOST1_SDIO:
								SD_HOST2_SD;
	if (enable)
		G_ipc_sdinfo[slot_id].from_ipc = enable;
	else
		memset(&G_ipc_sdinfo[slot_id], 0x0, sizeof(struct ipc_sdinfo));
}

EXPORT_SYMBOL(ambarella_sd_cmd_from_ipc);

void ambarella_sd_cd_ipc(int slot_id)
{
	struct mmc_host 			*mmc = G_mmc[slot_id];
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	mmc_detect_change(mmc, pslotinfo->plat_info->cd_delay);
}

EXPORT_SYMBOL(ambarella_sd_cd_ipc);
#else
//TODO: remove the ipc stuff, now just shut up the compiler
void ambarella_sd_cd_ipc(int slot_id)
{
}
EXPORT_SYMBOL(ambarella_sd_cd_ipc);
#endif

fs_initcall(ambarella_sd_init);
module_exit(ambarella_sd_exit);

MODULE_DESCRIPTION("Ambarella Media Processor SD/MMC Host Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

