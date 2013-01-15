
#if 1
/* This part is copied form drivers/mmc/host/ambarella_sd.c,
 * we add "fio lock" here for boss architecture.
 */

/*
 * drivers/mmc/host/boss_sdhc.c
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

/* ==========================================================================*/
#define CONFIG_SD_AMBARELLA_TIMEOUT_VAL		(0xe)
#define CONFIG_SD_AMBARELLA_WAIT_TIMEOUT	(HZ * 1)
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

	struct ambarella_sd_slot	slot_info;
	u32				slot_id;
	void				*pinfo;
	u32				valid;
	u16				nisen;

	struct notifier_block		system_event;
	struct semaphore		system_event_sem;
};

struct ambarella_sd_controller_info {
	unsigned char __iomem 		*regbase;
	struct device			*dev;
	struct resource			*mem;
	unsigned int			irq;
	spinlock_t			lock;

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
	char					*dmabuf;
	char					*sgbuf;
	u32					dma_start = 0xffffffff;
	u32					dma_end = 0;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	current_sg = pslotinfo->sg;
	dmabuf = pslotinfo->buf_vaddress;

	if (pslotinfo->sg_index == 0) {
		pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
			pslotinfo->slot_info.max_blk_sz);
		pslotinfo->dma_w_counter++;

		for (i = 0; i < pslotinfo->sg_len; i++) {
			current_sg[i].dma_address = dma_map_page(pinfo->dev,
							sg_page(&current_sg[i]),
							current_sg[i].offset,
							current_sg[i].length,
							DMA_TO_DEVICE);

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
		if (ambarella_sd_check_dma_boundary(pslotinfo, dma_start,
			(dma_end - dma_start), pslotinfo->dma_per_size) == 0) {
			pslotinfo->dma_need_fill = 1;
			ambsd_dbg(pslotinfo,
				"dma_start = 0x%x, dma_end = 0x%x\n",
				dma_start, dma_end);
		}

		if (pslotinfo->dma_need_fill) {
			if((pslotinfo->buf_vaddress == 0) ||
				(pslotinfo->buf_paddress == 0))
				BUG_ON(1);

			pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->slot_info.max_blk_sz);
			pslotinfo->dma_w_fill_counter++;

			for (i = 0; i < pslotinfo->sg_len; i++) {
				sgbuf = phys_to_virt(current_sg[i].dma_address);
				memcpy(dmabuf, sgbuf, current_sg[i].length);
				dmabuf += current_sg[i].length;
			}
			pslotinfo->dma_address = pslotinfo->buf_paddress;
			pslotinfo->dma_left = pslotinfo->dma_size;
			pslotinfo->sg_index = pslotinfo->sg_len;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |= pslotinfo->slot_info.max_blk_sz;
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
			pslotinfo->slot_info.max_blk_sz);
		pslotinfo->dma_r_counter++;

		for (i = 0; i < pslotinfo->sg_len; i++) {
			current_sg[i].dma_address = dma_map_page(pinfo->dev,
							sg_page(&current_sg[i]),
							current_sg[i].offset,
							current_sg[i].length,
							DMA_FROM_DEVICE);

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

		if (ambarella_sd_check_dma_boundary(pslotinfo, dma_start,
			(dma_end - dma_start), pslotinfo->dma_per_size) == 0) {
			pslotinfo->dma_need_fill = 1;
			ambsd_dbg(pslotinfo,
				"dma_start = 0x%x, dma_end = 0x%x\n",
				dma_start, dma_end);
		}

		if (pslotinfo->dma_need_fill) {
			if((pslotinfo->buf_vaddress == 0) ||
				(pslotinfo->buf_paddress == 0))
				BUG_ON(1);

			pslotinfo->dma_per_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->slot_info.max_blk_sz);
			pslotinfo->dma_r_fill_counter++;

			pslotinfo->dma_address = pslotinfo->buf_paddress;
			pslotinfo->dma_left = pslotinfo->dma_size;
			pslotinfo->sg_index = pslotinfo->sg_len;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |= pslotinfo->slot_info.max_blk_sz;
		} else {
			pslotinfo->dma_address =
				current_sg[pslotinfo->sg_index].dma_address;
			pslotinfo->dma_left =
				current_sg[pslotinfo->sg_index].length;
			pslotinfo->sg_index++;
			pslotinfo->blk_sz &= 0xFFF;
			pslotinfo->blk_sz |=
				ambarella_sd_dma_size_to_mask(
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
	char					*dmabuf;
	char					*sgbuf;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	current_sg = pslotinfo->sg;
	dmabuf = pslotinfo->buf_vaddress;

	for (i = 0; i < pslotinfo->sg_len; i++) {
		dma_unmap_page(pinfo->dev, current_sg[i].dma_address,
				current_sg[i].length, DMA_FROM_DEVICE);
		if (pslotinfo->dma_need_fill) {
			sgbuf = phys_to_virt(current_sg[i].dma_address);
			memcpy(sgbuf, dmabuf, current_sg[i].length);
			dmabuf += current_sg[i].length;
		}
	}
}
extern int ipc_vfio_select_lock(int module, int lock);
extern int ipc_vfio_unlock(int lock);
static void ambarella_sd_request_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	down(&pslotinfo->system_event_sem);
	if(pslotinfo->slot_id == 0){ //request for SD slot
		ipc_vfio_select_lock(3, 1);
	} else if(pslotinfo->slot_id == 1){ //request for SDIO slot
		ipc_vfio_select_lock(4, 1);
	} else {
		ambsd_warn(pslotinfo, "%s: invalid slot %d\n",__func__,pslotinfo->slot_id);
	}
	if (pslotinfo->slot_info.request)
		pslotinfo->slot_info.request();
}

static void ambarella_sd_release_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);

	if (pslotinfo->slot_info.release)
		pslotinfo->slot_info.release();

	ipc_vfio_unlock(1);
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
		msleep(1);
	}

	amba_writeb(pinfo->regbase + SD_TMO_OFFSET,
		CONFIG_SD_AMBARELLA_TIMEOUT_VAL);

	int_flag = pslotinfo->nisen	|
		SD_NISEN_REMOVAL	|
		SD_NISEN_INSERT		|
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

	if (pslotinfo->mrq && pslotinfo->mrq->cmd) {
		data = pslotinfo->mrq->cmd->data;
	} else {
		ambsd_err(pslotinfo, "%s mrq or cmd is NULL\n", __func__);
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
		if (eis & SD_EIS_CMD_BIT_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_CRC_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_TMOUT_ERR) {
			data->error = -ETIMEDOUT;
		} else {
			data->error = -EIO;
		}
		ambarella_sd_reset_data_line(pslotinfo->mmc);
	} else {
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

	if (pslotinfo->mrq && pslotinfo->mrq->cmd) {
		cmd = pslotinfo->mrq->cmd;
	} else {
		ambsd_err(pslotinfo, "%s mrq or cmd is NULL\n", __func__);
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

	if (pslotinfo->state == AMBA_SD_STATE_CMD) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
		wake_up(&pslotinfo->wait);
	}
}

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
		if (pslotinfo->slot_info.check_owner)
			enabled = pslotinfo->slot_info.check_owner();
		if (enabled)
			break;
	}

	if (unlikely(pslotinfo == NULL))
		goto ambarella_sd_irq_clear_irq;

	ambsd_dbg(pslotinfo, "%s nis = 0x%x, eis = 0x%x & %d:%d\n", __func__,
		nis, eis, pslotinfo->state, enabled);

	/* Check for card detection interrupt */
	if (!ambarella_is_valid_gpio_irq(&pslotinfo->slot_info.gpio_cd)) {
		if (nis & SD_NIS_REMOVAL) {
			ambsd_dbg(pslotinfo, "SD_NIS_REMOVAL\n");
			mmc_detect_change(pslotinfo->mmc, 0);
		} else if (nis & SD_NIS_INSERT) {
			if (pslotinfo->slot_info.cd_delay > 0) {
				mmc_detect_change(pslotinfo->mmc,
					pslotinfo->slot_info.cd_delay);
				ambsd_dbg(pslotinfo, "SD_NIS_INSERT %d...\n",
					pslotinfo->slot_info.cd_delay);
			} else {
				mmc_detect_change(pslotinfo->mmc, 5);
				ambsd_dbg(pslotinfo, "SD_NIS_INSERT...\n");
			}
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

static irqreturn_t ambarella_sd_gpio_cd_irq(int irq, void *devid)
{
	struct ambarella_sd_mmc_info		*pslotinfo;
//	u32					val = 0;
	struct ambarella_sd_controller_info	*pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)devid;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (pslotinfo->valid &&
		ambarella_is_valid_gpio_irq(&pslotinfo->slot_info.gpio_cd)) {
#if 0 // remove unneeded gpio access
		ambarella_gpio_config(pslotinfo->slot_info.gpio_cd.irq_gpio,
			GPIO_FUNC_SW_INPUT);
		val = ambarella_gpio_get(pslotinfo->slot_info.gpio_cd.irq_gpio);
		ambarella_gpio_config(pslotinfo->slot_info.gpio_cd.irq_gpio,
			pslotinfo->slot_info.gpio_cd.irq_gpio_mode);

		ambsd_dbg(pslotinfo, "%s:%d\n",
			(val == pslotinfo->slot_info.gpio_cd.irq_gpio_val) ?
			"card insert" : "card eject",
			pslotinfo->slot_info.cd_delay);
#endif
		if (pslotinfo->slot_info.cd_delay > 0) {
			mmc_detect_change(pslotinfo->mmc,
				pslotinfo->slot_info.cd_delay);
		} else {
			mmc_detect_change(pslotinfo->mmc, 5);
		}
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
		ambarella_set_gpio_output(&pslotinfo->slot_info.ext_reset, 1);
		ambarella_set_gpio_output(&pslotinfo->slot_info.ext_power, 0);
	} else if (pwr_mode == MMC_POWER_UP) {
		ambarella_set_gpio_output(&pslotinfo->slot_info.ext_power, 1);
		ambarella_set_gpio_output(&pslotinfo->slot_info.ext_reset, 0);
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
			msleep(pslotinfo->slot_info.ext_power.active_delay);
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
	if (bus_width == MMC_BUS_WIDTH_4) {
		hostr &= ~(SD_HOST_8BIT);
		hostr |= SD_HOST_4BIT;
	} else if (bus_width == MMC_BUS_WIDTH_1) {
		hostr &= ~(SD_HOST_8BIT);
		hostr &= ~(SD_HOST_4BIT);
	}
	hostr &= ~SD_HOST_HIGH_SPEED;
	if (timing == MMC_TIMING_SD_HS) {
		ambsd_dbg(pslotinfo, "MMC_TIMING_SD_HS!\n");
		//hostr |= SD_HOST_HIGH_SPEED;
	}
	amba_writeb(pinfo->regbase + SD_HOST_OFFSET, hostr);
	ambsd_dbg(pslotinfo, "hostr = 0x%x.\n", hostr);
}

static void ambarella_sd_check_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info		*pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if ((pinfo->controller_ios.power_mode != ios->power_mode) ||
		(pinfo->controller_ios.vdd != ios->vdd) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)){
		ambarella_sd_set_pwr(mmc, ios->power_mode, ios->vdd);
		pinfo->controller_ios.power_mode = ios->power_mode;
		pinfo->controller_ios.vdd = ios->vdd;
	}

	if ((pinfo->controller_ios.clock != ios->clock) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		msleep(10);
		ambarella_sd_set_clk(mmc, ios->clock);
		pinfo->controller_ios.clock = ios->clock;
		msleep(10);
	}

	if ((pinfo->controller_ios.bus_width != ios->bus_width) ||
		(pinfo->controller_ios.timing != ios->timing) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)){
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

	if (pslotinfo->slot_info.gpio_wp.gpio_id != -1) {
		wpspl = !ambarella_get_gpio_input(
			&pslotinfo->slot_info.gpio_wp);
	} else {
		wpspl = amba_readl(pinfo->regbase + SD_STA_OFFSET);
		ambsd_dbg(pslotinfo, "SD/MMC RD[0x%x].\n", wpspl);
		wpspl &= SD_STA_WPS_PL;
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
	u32					error_id = -ETIMEDOUT;
	struct ambarella_sd_controller_info	*pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_request_bus(mmc);
	ambarella_sd_check_ios(mmc, &mmc->ios);

	pslotinfo->mrq = mrq;

	card_sta = amba_readl(pinfo->regbase + SD_STA_OFFSET);

	if ((card_sta & SD_STA_CARD_INSERTED) ||
		ambarella_is_valid_gpio_irq(&pslotinfo->slot_info.gpio_cd)) {
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
		ambsd_dbg(pslotinfo, "card_sta 0x%x.\n", card_sta);
		mrq->cmd->error = error_id;
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
		ambsd_dbg(pslotinfo, "need_reset %d %d 0x%x %d!\n",
			pslotinfo->state, mrq->cmd->opcode, card_sta, error_id);
		ambarella_sd_reset_all(pslotinfo->mmc);
	}

	pslotinfo->mrq = NULL;

	ambarella_sd_release_bus(mmc);

	mmc_request_done(mmc, mrq);
}

static const struct mmc_host_ops ambarella_sd_host_ops = {
	.request	= ambarella_sd_request,
	.set_ios	= ambarella_sd_ios,
	.get_ro		= ambarella_sd_get_ro,
	.enable_sdio_irq= ambarella_sd_enable_sdio_irq,
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

	case AMBA_EVENT_PRE_TOSS:
		pr_debug("%s[%d]: AMBA_EVENT_PRE_TOSS\n",
			__func__, pslotinfo->slot_id);
#ifdef CONFIG_PM
		if (pslotinfo->mmc) {
			pm_message_t state;
			state.event = 2;
			pslotinfo->state = AMBA_SD_STATE_RESET;
			errorCode = mmc_suspend_host(pslotinfo->mmc);
			if (errorCode)
				ambsd_err(pslotinfo,
					"Can't mmc_suspend_host!\n");
		}
#endif
		break;

	case AMBA_EVENT_POST_TOSS:
		pr_debug("%s[%d]: AMBA_EVENT_POST_TOSS\n",
			__func__, pslotinfo->slot_id);
#ifdef CONFIG_PM
		if (pslotinfo->mmc) {
			errorCode = mmc_resume_host(pslotinfo->mmc);
			if (errorCode)
				ambsd_err(pslotinfo,
					"Can't mmc_resume_host!\n");
		}
#endif
		break;

	default:
		break;
	}

	return errorCode;
}

/* ==========================================================================*/
static int __devinit ambarella_sd_probe(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo = NULL;
	struct resource 			*irq;
	struct resource 			*mem;
	struct resource 			*ioarea;
	struct mmc_host				*mmc;
	u32					hc_cap;
	u32					i;
	u32					clock_min;

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
		memcpy(&pslotinfo->slot_info, &pinfo->pcontroller->slot[i],
			sizeof(struct ambarella_sd_slot));
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

		mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ;
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

		if (pslotinfo->slot_info.use_bounce_buffer) {
			//mmc->max_hw_segs = 128;
			//mmc->max_phys_segs = 128;
			//2.6.38 merged max_hw_segs & max_phys_segs into max_segs
			mmc->max_segs = 128;
			mmc->max_seg_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->slot_info.max_blk_sz);
			mmc->max_req_size = mmc->max_seg_size;
			mmc->max_blk_count = 0xFFFF;

			pslotinfo->buf_vaddress = dma_alloc_coherent(
				pinfo->dev, mmc->max_seg_size,
				&pslotinfo->buf_paddress, GFP_KERNEL);
			if (!pslotinfo->buf_vaddress) {
				ambsd_err(pslotinfo, "Can't alloc DMA memory");
				errorCode = -ENOMEM;
				goto sd_errorCode_free_host;
			}

			if (ambarella_sd_check_dma_boundary(pslotinfo,
				pslotinfo->buf_paddress,
				mmc->max_seg_size,
				ambarella_sd_dma_mask_to_size(
				pslotinfo->slot_info.max_blk_sz)) == 0) {
				ambsd_err(pslotinfo, "DMA boundary err!\n");
				errorCode = -ENOMEM;
				goto sd_errorCode_free_host;
			}
			dev_notice(&pdev->dev, "Slot%d bounce buffer:"
				"0x%p<->0x%08x, size=%d\n", pslotinfo->slot_id,
				pslotinfo->buf_vaddress,
				pslotinfo->buf_paddress,
				mmc->max_seg_size);
		} else {
			//mmc->max_hw_segs = 1;
			//mmc->max_phys_segs = 1;
			//2.6.38 merged max_hw_segs & max_phys_segs into max_segs
			mmc->max_segs = 128;
			mmc->max_seg_size = ambarella_sd_dma_mask_to_size(
				pslotinfo->slot_info.max_blk_sz);
			mmc->max_req_size = mmc->max_seg_size;
			mmc->max_blk_count = 0xFFFF;

			pslotinfo->buf_paddress = (dma_addr_t)NULL;
			pslotinfo->buf_vaddress = NULL;
		}

		if (pslotinfo->slot_info.ext_power.gpio_id != -1) {
			errorCode = gpio_request(
				pslotinfo->slot_info.ext_power.gpio_id,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get Power GPIO%d\n",
				pslotinfo->slot_info.ext_power.gpio_id);
				goto sd_errorCode_free_host;
			}
		}

		if (pslotinfo->slot_info.ext_reset.gpio_id != -1) {
			errorCode = gpio_request(
				pslotinfo->slot_info.ext_reset.gpio_id,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get Reset GPIO%d\n",
				pslotinfo->slot_info.ext_reset.gpio_id);
				goto sd_errorCode_free_host;
			}
		}

		if (pslotinfo->slot_info.gpio_wp.gpio_id != -1) {
			errorCode = gpio_request(
				pslotinfo->slot_info.gpio_wp.gpio_id,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get WP GPIO%d\n",
				pslotinfo->slot_info.gpio_wp.gpio_id);
				goto sd_errorCode_free_host;
			}
		}
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

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		errorCode = mmc_add_host(pslotinfo->mmc);
		if (errorCode) {
			ambsd_err(pslotinfo, "Can't add mmc host!\n");
			goto sd_errorCode_remove_host;
		}
		pslotinfo->valid = 1;
		pslotinfo->system_event.notifier_call =
			ambarella_sd_system_event;
		ambarella_register_event_notifier(&pslotinfo->system_event);

		if (ambarella_is_valid_gpio_irq(
			&pslotinfo->slot_info.gpio_cd)) {
			errorCode = gpio_request(
				pslotinfo->slot_info.gpio_cd.irq_gpio,
				pdev->name);
			if (errorCode < 0) {
				ambsd_err(pslotinfo, "Can't get CD GPIO%d\n",
					pslotinfo->slot_info.gpio_cd.irq_gpio);
				goto sd_errorCode_free_host;
			}

			ambarella_gpio_config(
				pslotinfo->slot_info.gpio_cd.irq_gpio,
				pslotinfo->slot_info.gpio_cd.irq_gpio_mode);
			errorCode = request_irq(
				pslotinfo->slot_info.gpio_cd.irq_line,
				ambarella_sd_gpio_cd_irq,
				pslotinfo->slot_info.gpio_cd.irq_type,
				dev_name(&pdev->dev), pslotinfo);
			if (errorCode) {
				ambsd_err(pslotinfo,
					"Can't Request GPIO(%d) CD IRQ(%d)!\n",
					pslotinfo->slot_info.gpio_cd.irq_gpio,
					pslotinfo->slot_info.gpio_cd.irq_line);
				goto sd_errorCode_free_host;
			}
		}
	}

	platform_set_drvdata(pdev, pinfo);


	dev_notice(&pdev->dev,
		"Ambarella Media Processor SD/MMC[%d] probed %d slots!\n",
		pdev->id, pinfo->pcontroller->num_slots);

	goto sd_errorCode_na;

sd_errorCode_remove_host:
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if ((pslotinfo->mmc) && (pslotinfo->valid == 1))
			mmc_remove_host(pslotinfo->mmc);
	}

	free_irq(pinfo->irq, pinfo);

sd_errorCode_free_host:
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];

		if (ambarella_is_valid_gpio_irq(
			&pslotinfo->slot_info.gpio_cd)) {
			free_irq(pslotinfo->slot_info.gpio_cd.irq_line,
				pslotinfo);
			gpio_free(pslotinfo->slot_info.gpio_cd.irq_gpio);
		}

		if (pslotinfo->slot_info.gpio_wp.gpio_id != -1) {
			gpio_free(pslotinfo->slot_info.gpio_wp.gpio_id);
		}

		if (pslotinfo->slot_info.ext_power.gpio_id != -1) {
			gpio_free(pslotinfo->slot_info.ext_power.gpio_id);
		}

		if (pslotinfo->slot_info.ext_reset.gpio_id != -1) {
			gpio_free(pslotinfo->slot_info.ext_reset.gpio_id);
		}

		if (pslotinfo->buf_vaddress) {
			dma_free_coherent(pinfo->dev,
				pslotinfo->mmc->max_seg_size,
				pslotinfo->buf_vaddress,
				pslotinfo->buf_paddress);
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

			ambarella_unregister_event_notifier(
				&pslotinfo->system_event);

			if (ambarella_is_valid_gpio_irq(
				&pslotinfo->slot_info.gpio_cd)) {
				free_irq(pslotinfo->slot_info.gpio_cd.irq_line,
					pslotinfo);
				gpio_free(
					pslotinfo->slot_info.gpio_cd.irq_gpio);
			}

			if (pslotinfo->mmc)
				mmc_remove_host(pslotinfo->mmc);
		}

		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];

			if (pslotinfo->slot_info.ext_power.gpio_id != -1) {
				gpio_free(
				pslotinfo->slot_info.ext_power.gpio_id);
			}

			if (pslotinfo->slot_info.ext_reset.gpio_id != -1) {
				gpio_free(
				pslotinfo->slot_info.ext_reset.gpio_id);
			}

			if (pslotinfo->slot_info.gpio_wp.gpio_id != -1) {
				gpio_free(
				pslotinfo->slot_info.gpio_wp.gpio_id);
			}

			if (pslotinfo->buf_vaddress) {
				dma_free_coherent(pinfo->dev,
					pslotinfo->mmc->max_seg_size,
					pslotinfo->buf_vaddress,
					pslotinfo->buf_paddress);
				pslotinfo->buf_vaddress = NULL;
				pslotinfo->buf_paddress = (dma_addr_t)NULL;
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
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;

	pinfo = platform_get_drvdata(pdev);
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			pslotinfo->state = AMBA_SD_STATE_RESET;
			errorCode = mmc_suspend_host(pslotinfo->mmc);
			if (errorCode)
				ambsd_err(pslotinfo,
					"Can't mmc_suspend_host!\n");
		}
	}

	if (!device_may_wakeup(&pdev->dev)) {
		disable_irq(pinfo->irq);
		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			if (ambarella_is_valid_gpio_irq(
				&pslotinfo->slot_info.gpio_cd)) {
				disable_irq(
					pslotinfo->slot_info.gpio_cd.irq_line);
			}
		}
	}

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);
	return errorCode;
}

static int ambarella_sd_resume(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_sd_controller_info	*pinfo;
	struct ambarella_sd_mmc_info		*pslotinfo;
	u32					i;

	pinfo = platform_get_drvdata(pdev);

	pinfo->pcontroller->set_pll(pinfo->pcontroller->max_clk);

	if (!device_may_wakeup(&pdev->dev)) {
		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			ambarella_sd_reset_all(pslotinfo->mmc);
			if (ambarella_is_valid_gpio_irq(
				&pslotinfo->slot_info.gpio_cd)) {
				enable_irq(
					pslotinfo->slot_info.gpio_cd.irq_line);
			}
		}
		enable_irq(pinfo->irq);
	}

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			errorCode = mmc_resume_host(pslotinfo->mmc);
			if (errorCode)
				ambsd_err(pslotinfo,
					"Can't mmc_resume_host!\n");
		}
	}

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

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

	errorCode = platform_driver_register(&ambarella_sd_driver);
	if (errorCode)
		printk(KERN_ERR "Register ambarella_sd_driver failed %d!\n",
			errorCode);

	return errorCode;
}

static void __exit ambarella_sd_exit(void)
{
	platform_driver_unregister(&ambarella_sd_driver);
}

module_init(ambarella_sd_init);
module_exit(ambarella_sd_exit);

MODULE_DESCRIPTION("Ambarella Media Processor SD/MMC Host Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");


#else
/*
 * drivers/mmc/host/ambarella_sd.c
 *
 * Authors:
 *	Charles Chiou <cchiou@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Copyright (C) 2009-2010, Ambarella Inc.
 */

//#define DEBUG

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <linux/aipc/i_sdhc.h>

#include <linux/module.h>

#include <linux/aipc/aipc.h>



#define MAX_SDHC_HOST		2
#define MAX_SDHC_CARD_PER_HOST	2

struct boss_sdhc_s;

/*
 * This object is bound to each instance of mmc_host, i.e., it is created
 * as a by-product of mmc_alloc_host() call, and accessed via mmc_priv(mmc).
 */
struct boss_mmc_host_s
{
	struct mmc_host			*mmc;
	struct boss_sdhc_s *sdhc;
	int host;
	int card;
	unsigned char bus_width;	/* mmc_ios.bus_width */
	unsigned char timing;		/* mmc_ios.timing */

	int present;
	int iocard;

	int sdioirqen;
};

/*
 * The virtual SDHC driver object - bound to the platform device, and contains
 * the boss_mmc_host objects as a 2D array of:
 *	MAX_SDHC_HOST x MAX_SDHC_CARD_PER_HOST.
 */
struct boss_sdhc_s
{
	int num_host;
	int num_mmc;
	struct boss_mmc_host_s *mh[MAX_SDHC_HOST][MAX_SDHC_CARD_PER_HOST];
	struct device			*dev;
	struct semaphore sem;
	int inhibit_storage_cards;
};

#if defined(CONFIG_PROC_FS)

extern struct proc_dir_entry *proc_aipc;

static struct proc_dir_entry *proc_vsdhc = NULL;
static struct proc_dir_entry *proc_inhibit = NULL;
static struct proc_dir_entry *proc_status = NULL;

static int boss_sdhc_inhibit_proc_read(char *page, char **start, off_t off,
				       int count, int *eof, void *data)
{
	struct boss_sdhc_s *sdhc = (struct boss_sdhc_s *) data;
	int len = 0;

	len += snprintf(page + len, PAGE_SIZE - len,
			"%d\n", sdhc->inhibit_storage_cards);
	*eof = 1;

	return len;
}

static int boss_sdhc_inhibit_proc_write(struct file *file,
					const char __user *buffer,
					unsigned long count, void *data)
{
	struct boss_sdhc_s *sdhc = (struct boss_sdhc_s *) data;
	char *buf;
	int val;
	int i, j, changed = 0;

	buf = kmalloc(GFP_KERNEL, count);
	if (buf == NULL)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	if (sscanf(buf, "%d", &val) == 1) {
		down(&sdhc->sem);

		val = val ? 1 : 0;
		if (sdhc->inhibit_storage_cards != val)
			changed = 1;
		sdhc->inhibit_storage_cards = val;

		if (changed) {
			for (i = 0; i < MAX_SDHC_HOST; i++) {
				for (j = 0; j < MAX_SDHC_CARD_PER_HOST; j++) {
					if (sdhc->mh[i][j] != NULL &&
					    sdhc->mh[i][i]->iocard == 0)
						mmc_detect_change(
							sdhc->mh[i][j]->mmc, 0);
				}
			}
		}

		up(&sdhc->sem);
	}

	kfree(buf);

	return count;
}

static int boss_sdhc_status_proc_read(char *page, char **start, off_t off,
				       int count, int *eof, void *data)
{
	struct boss_sdhc_s *sdhc = (struct boss_sdhc_s *) data;
	int len = 0;
	int i, j, k = 0;

	down(&sdhc->sem);

	len += snprintf(page + len, PAGE_SIZE - len,
			"num host = %d\n"
			"num mmc  = %d\n",
			sdhc->num_host, sdhc->num_mmc);
	for (i = 0; i < MAX_SDHC_HOST; i++) {
		for (j = 0; j < MAX_SDHC_CARD_PER_HOST; j++) {
			if (sdhc->mh[i][j] == NULL)
				continue;
			len += snprintf(page + len, PAGE_SIZE - len,
					"--- instance #%d on %d:%d ---\n",
					k, i, j);
			len += snprintf(page + len, PAGE_SIZE - len,
					"present: %d\n"
					"iocard:  %d\n",
					sdhc->mh[i][j]->present,
					sdhc->mh[i][j]->iocard);
			k++;
		}
	}

	up(&sdhc->sem);

	*eof = 1;

	return len;
}

static void boss_sdhc_procfs_init(struct boss_sdhc_s *sdhc)
{
	proc_vsdhc = proc_mkdir("vsdhc", proc_aipc);
	if (proc_vsdhc == NULL) {
		pr_err("create vsdhc dir failed!\n");
		return;
	}

	proc_inhibit = create_proc_entry("inhibit_storage_cards", 0,
					 proc_vsdhc);
	if (proc_inhibit) {
		proc_inhibit->data = sdhc;
		proc_inhibit->read_proc = boss_sdhc_inhibit_proc_read;
		proc_inhibit->write_proc = boss_sdhc_inhibit_proc_write;
	}

	proc_status = create_proc_entry("status", 0,
					proc_vsdhc);
	if (proc_status) {
		proc_status->data = sdhc;
		proc_status->read_proc = boss_sdhc_status_proc_read;
		proc_status->write_proc = NULL;
	}
}

static void boss_sdhc_procfs_cleanup(void)
{
	if (proc_status) {
		remove_proc_entry("status", proc_vsdhc);
		proc_status = NULL;
	}

	if (proc_inhibit) {
		remove_proc_entry("inhibit_storage_cards", proc_vsdhc);
		proc_inhibit = NULL;
	}

	if (proc_vsdhc) {
		remove_proc_entry("vsdhc", proc_aipc);
		proc_vsdhc = NULL;
		}
	}

#endif  /* CONFIG_PROC_FS */

/*
 * Virtual SDHC event handler.
 */
static void boss_sdhc_event_handler(int host, int card, int event, int type,
				    void *arg)
{
	struct boss_sdhc_s *sdhc = (struct boss_sdhc_s *) arg;

	if (host < 0 || host >= MAX_SDHC_HOST ||
	    card < 0 || card >= MAX_SDHC_CARD_PER_HOST)
		return;	/* Sanity check */

	if (event == IPC_SDHC_EVENT_INSERT)
		sdhc->mh[host][card]->present = 1;
	else
		sdhc->mh[host][card]->present = 0;

	if (type == IPC_SDHC_TYPE_IOCARD)
		sdhc->mh[host][card]->iocard = 1;
	else
		sdhc->mh[host][card]->iocard = 0;

	mmc_detect_change(sdhc->mh[host][card]->mmc, 0);
}

/*
 * Virtual SDHC SDIO IRQ handler.
 */
static void boss_sdhc_sdio_irq_handler(int host, void *arg)
{
	struct boss_sdhc_s *sdhc = (struct boss_sdhc_s *) arg;
	int card;
	unsigned long flags;

	if (host < 0 || host >= MAX_SDHC_HOST)
		return;	/* Sanity check */

	local_irq_save(flags);
	for (card = 0; card < MAX_SDHC_CARD_PER_HOST; card++) {
		if (sdhc->mh[host][card]->sdioirqen) {
			mmc_signal_sdio_irq(sdhc->mh[host][card]->mmc);
		}
	}
	local_irq_restore(flags);
}

/*
 * Map/translate Linux' mrq->[cmd|stop]->flags to the virtual driver.
 */
static inline unsigned int boss_sdhc_map_expect(unsigned int flags)
{

	if ((flags & MMC_RSP_PRESENT) == 0)
		return IPC_SDHC_EXPECT_NONE;

	if (flags & MMC_RSP_136) {
		if (flags & MMC_RSP_CRC)
			return IPC_SDHC_EXPECT_R2;
		else
			BUG();
	}

	if (((MMC_RSP_BUSY | MMC_RSP_OPCODE | MMC_RSP_CRC) & flags) ==
	    ( MMC_RSP_BUSY | MMC_RSP_OPCODE | MMC_RSP_CRC))
		return IPC_SDHC_EXPECT_R1B;

	if (((MMC_RSP_OPCODE | MMC_RSP_CRC) & flags) ==
	    ( MMC_RSP_OPCODE | MMC_RSP_CRC))
		return IPC_SDHC_EXPECT_R1;

	return IPC_SDHC_EXPECT_R3;
}

/*
 * Map/translate Linux' mrq->data->flags to the virtual driver.
 */
static inline unsigned int boss_sdhc_map_data_flag(unsigned int flags,
						   struct boss_mmc_host_s *mh)
{
	unsigned int f = 0x0;

	if (flags & MMC_DATA_WRITE)
		f |= IPC_SDHC_DATA_WRITE;
	if (flags & MMC_DATA_READ)
		f |= IPC_SDHC_DATA_READ;
	if (flags == MMC_DATA_STREAM)
		f |= IPC_SDHC_DATA_STREAM;

	if (mh->bus_width == MMC_BUS_WIDTH_1)
		f |= IPC_SDHC_DATA_1BIT_WIDTH;
	else if (mh->bus_width == MMC_BUS_WIDTH_4)
		f |= IPC_SDHC_DATA_4BIT_WIDTH;
	else if (mh->bus_width == MMC_BUS_WIDTH_8)
		f |= IPC_SDHC_DATA_8BIT_WIDTH;

	if (mh->timing != MMC_TIMING_LEGACY)
		f |= IPC_SDHC_DATA_HIGH_SPEED;

	return f;
}

/*
 * Map/translate Linux' ios->clock to the virtual driver.
 */
static inline unsigned int boss_sdhc_map_clock(unsigned int clock)
{
	if (clock == 0)
		return 400;
	return clock;	/* 1-to-1 :> */
}

/*
 * Map/translate Linux' ios->vdd to the virtual driver.
 */
static inline unsigned int boss_sdhc_map_vdd(unsigned short vdd)
{
	switch (0x1 << vdd) {
	case MMC_VDD_165_195:	return IPC_SDHC_VDD_190;
	case MMC_VDD_20_21:	return IPC_SDHC_VDD_210;
	case MMC_VDD_21_22:	return IPC_SDHC_VDD_220;
	case MMC_VDD_22_23:	return IPC_SDHC_VDD_230;
	case MMC_VDD_23_24:	return IPC_SDHC_VDD_240;
	case MMC_VDD_24_25:	return IPC_SDHC_VDD_250;
	case MMC_VDD_25_26:	return IPC_SDHC_VDD_260;
	case MMC_VDD_26_27:	return IPC_SDHC_VDD_270;
	case MMC_VDD_27_28:	return IPC_SDHC_VDD_280;
	case MMC_VDD_28_29:	return IPC_SDHC_VDD_290;
	case MMC_VDD_29_30:	return IPC_SDHC_VDD_300;
	case MMC_VDD_30_31:	return IPC_SDHC_VDD_310;
	case MMC_VDD_31_32:	return IPC_SDHC_VDD_320;
	case MMC_VDD_32_33:	return IPC_SDHC_VDD_330;
	case MMC_VDD_33_34:	return IPC_SDHC_VDD_340;
	case MMC_VDD_34_35:	return IPC_SDHC_VDD_350;
	case MMC_VDD_35_36:	return IPC_SDHC_VDD_360;
	default:		return IPC_SDHC_VDD_330;
	}

	return IPC_SDHC_VDD_330;
}

/*
 * Map/translate Linux' ios->bus_mode to the virtual driver.
 */
static inline unsigned int boss_sdhc_map_bus_mode(unsigned char bus_mode)
{
	return (unsigned int) bus_mode;	/* 1-to-1 :> */
}

/*
 * Map/translate Linux' ios->power_mode to the virtual driver.
 */
static inline unsigned int boss_sdhc_map_power_mode(unsigned char power_mode)
{
	return (unsigned int) power_mode; /* 1-to-1 :> */
}

/*
 * Map translate virtual driver's to sdmmc_cmd.errror to Linux MMC stack.
 */
static inline unsigned int boss_sdhc_map_error(unsigned int error)
{
	switch (error) {
	case IPC_SDHC_ERR_NONE:
		return 0;
	case IPC_SDHC_ERR_NO_CARD:
		return -ENOMEDIUM;
	case IPC_SDHC_ERR_TIMEOUT:
	case IPC_SDHC_ERR_ISR_TIMEOUT:
	case IPC_SDHC_ERR_CLINE_TIMEOUT:
		return -ETIMEDOUT;
	case IPC_SDHC_ERR_BADCRC:
	case IPC_SDHC_ERR_FAILED:
	case IPC_SDHC_ERR_UNUSABLE:
	case IPC_SDHC_ERR_CHECK_PATTERN:
		return -EILSEQ;
	case IPC_SDHC_ERR_INVALID:
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

/*
 * Command request.
 */
static void boss_sdhc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct boss_mmc_host_s *mh = (struct boss_mmc_host_s *) mmc_priv(mmc);
	struct boss_sdhc_s *sdhc = mh->sdhc;
	struct ipc_sdhc_command cmd;
	struct ipc_sdhc_command stop;
	struct ipc_sdhc_data data;
	struct ipc_sdhc_req req;
	struct ipc_sdhc_reply reply;

	down(&sdhc->sem);

	/* If card is not present, then don't perform any transactions! */
	if (mh->present == 0) {
		if (mrq->cmd)
			mrq->cmd->error = -ENOMEDIUM;
		if (mrq->data)
			mrq->data->error = -ENOMEDIUM;
		if (mrq->stop)
			mrq->stop->error = -ENOMEDIUM;
		goto done;
	}

	/*
	 * If card is present, but is storage card, and 'inhibit_storage_cards'
	 * variable is set, we fail as if there is no card in the slot.
	 */
	if (mh->iocard == 0 && sdhc->inhibit_storage_cards) {
		if (mrq->cmd)
			mrq->cmd->error = -ENOMEDIUM;
		if (mrq->data)
			mrq->data->error = -ENOMEDIUM;
		if (mrq->stop)
			mrq->stop->error = -ENOMEDIUM;
		goto done;
	}

	memset(&req, 0x0, sizeof(req));

	if (mrq->cmd) {
		cmd.opcode = mrq->cmd->opcode;
		cmd.arg = mrq->cmd->arg;
		cmd.expect = boss_sdhc_map_expect(mrq->cmd->flags);
		cmd.retries = mrq->cmd->retries + 1;
		cmd.to = 1000;
		req.cmd = &cmd;
		}

	if (mrq->data) {
		BUG_ON(mrq->data->sg_len != 1);
		data.buf = (char *) sg_phys(&mrq->data->sg[0]);
		data.timeout_ns = mrq->data->timeout_ns;
		data.timeout_clks = mrq->data->timeout_clks;
		data.blksz = mrq->data->blksz;
		data.blocks = mrq->data->blocks;
		data.flags = boss_sdhc_map_data_flag(mrq->data->flags, mh);
		req.data = &data;
		}

	if (mrq->stop) {
		stop.opcode = mrq->stop->opcode;
		stop.arg = mrq->stop->arg;
		stop.expect = boss_sdhc_map_expect(mrq->stop->flags);
		stop.retries = mrq->stop->retries + 1;
		stop.to = 1000;
		req.stop = &stop;
		}

	ipc_sdhc_req(mh->host, mh->card, &req, &reply);

	if (mrq->cmd) {
		mrq->cmd->error = boss_sdhc_map_error(reply.cmd_error);
		if ((mrq->cmd->flags & MMC_RSP_136)) {
			mrq->cmd->resp[0] = reply.cmd_resp[0];
			mrq->cmd->resp[1] = reply.cmd_resp[1];
			mrq->cmd->resp[2] = reply.cmd_resp[2];
			mrq->cmd->resp[3] = reply.cmd_resp[3];
		} else {
				mrq->cmd->resp[0]  = (reply.cmd_resp[3] >>  8);
				mrq->cmd->resp[0] |= (reply.cmd_resp[2] << 24);
		}

		pr_debug("CMD %2d(0x%.8x) flags %d\n",
			 mrq->cmd->opcode,
			 mrq->cmd->arg,
			 mrq->cmd->flags);
		pr_debug("\t-> %d [0x%.8x 0x%.8x 0x%.8x 0x%.8x]\n",
			 mrq->cmd->error,
			 mrq->cmd->resp[0],
			 mrq->cmd->resp[1],
			 mrq->cmd->resp[2],
			 mrq->cmd->resp[3]);
		}




	if (mrq->data) {
		mrq->data->error = boss_sdhc_map_error(reply.data_error);
		mrq->data->bytes_xfered = reply.data_bytes_xfered;
	}

	if (mrq->stop) {
		mrq->stop->error = boss_sdhc_map_error(reply.stop_error);
		if ((mrq->stop->flags & MMC_RSP_136)) {
			mrq->stop->resp[0] = reply.stop_resp[0];
			mrq->stop->resp[1] = reply.stop_resp[1];
			mrq->stop->resp[2] = reply.stop_resp[2];
			mrq->stop->resp[3] = reply.stop_resp[3];
	} else {
			mrq->stop->resp[0]  = (reply.stop_resp[3] >>  8);
			mrq->stop->resp[0] |= (reply.stop_resp[2] << 24);
		}
	}

done:

	up(&sdhc->sem);

	mmc_request_done(mmc, mrq);
}

/*
 * Bus request.
 */
static void boss_sdhc_set_ios(struct mmc_host *mmc, struct mmc_ios *mios)
{
	struct boss_mmc_host_s *mh = (struct boss_mmc_host_s *) mmc_priv(mmc);
	struct boss_sdhc_s *sdhc = mh->sdhc;
	struct ipc_sdhc_ios vios;
	int rval, actual_clock;

	down(&sdhc->sem);

	vios.desired_clock = boss_sdhc_map_clock(mios->clock);
	vios.vdd = boss_sdhc_map_vdd(mios->vdd);
	vios.bus_mode = boss_sdhc_map_bus_mode(mios->bus_mode);
	vios.power_mode = boss_sdhc_map_power_mode(mios->power_mode);
	mh->bus_width = mios->bus_width;
	mh->timing = mios->timing;

	rval = ipc_sdhc_set_ios(mh->host, mh->card, &vios);
	if (rval >= 0) 
		actual_clock = rval;

	up(&sdhc->sem);
}

/*
 * Check write-protect.
 */
static int boss_sdhc_get_ro(struct mmc_host *mmc)
{
	struct boss_mmc_host_s *mh = (struct boss_mmc_host_s *) mmc_priv(mmc);
	struct boss_sdhc_s *sdhc = mh->sdhc;
	int rval;

	down(&sdhc->sem);
	rval = ipc_sdhc_write_protect(mh->host, mh->card);
	up(&sdhc->sem);

	return rval < 0 ? 0 : rval;
}

/*
 * Enable SDIO irq.
 */
static void boss_sdhc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct boss_mmc_host_s *mh = (struct boss_mmc_host_s *) mmc_priv(mmc);

	if (enable == 0) {
		/* The VSDHC already disabled the SDIO IRQ so we don't */
		/* need to disable it from our side and waste an IPC */
		mh->sdioirqen = 0;
	} else {
		/* Enable SDIO IRQ */
		mh->sdioirqen = 1;
		ipc_sdhc_enable_sdio_irq(mh->host, 1);
	}
}

/*
 * Host controller operation functions.
 */
static struct mmc_host_ops boss_sdhc_ops = {
	.request =	boss_sdhc_request,
	.set_ios =	boss_sdhc_set_ios,
	.get_ro =	boss_sdhc_get_ro,
	.enable_sdio_irq =	boss_sdhc_enable_sdio_irq,
};

/*
 * BOSS SDHC driver probing.
 */
static int __devinit boss_sdhc_probe(struct platform_device *pdev)
{
	struct boss_sdhc_s *sdhc = NULL;
	struct boss_mmc_host_s *mh = NULL;
	struct mmc_host *mmc = NULL;
	int rval = 0;
	int i, j, n;


	/* Allocate memory for driver */
	sdhc = kzalloc(sizeof(*sdhc), GFP_KERNEL);
	if (sdhc == NULL) {
		rval = -ENOMEM;
		goto done;
	}

	platform_set_drvdata(pdev, sdhc);
	sdhc->dev = &pdev->dev;
	sema_init(&sdhc->sem, 1);
	sdhc->inhibit_storage_cards = 1;

	/* Get number of actual hosts */
	sdhc->num_host = ipc_sdhc_num_host();

	/* Allocate an mmc_host object for each slot on a host */
	for (i = 0; i < sdhc->num_host && i < MAX_SDHC_HOST; i++) {
		n = ipc_sdhc_num_slot(i);
		for (j = 0; j < n && j < MAX_SDHC_CARD_PER_HOST; j++) {
			mmc = mmc_alloc_host(sizeof(*mh), sdhc->dev);
			if (mmc == NULL) {
				rval = -ENOMEM;
				goto done;
			}

			/* Set up mh variables */
			mh = mmc_priv(mmc);
			mh->mmc = mmc;
			mh->sdhc = sdhc;
			mh->host = i;
			mh->card = j;
			mh->present = ipc_sdhc_card_in_slot(i, j);
			mh->iocard = ipc_sdhc_has_iocard(i, j);

			/* Link the mh array to the actual mh address */
			sdhc->mh[i][j] = mh;

			/* Increment instance count */
			sdhc->num_mmc++;

			/* Set up the mmc_host properties */
			mmc->ops = &boss_sdhc_ops;
			mmc->ocr_avail  = MMC_VDD_165_195;
			mmc->ocr_avail |= MMC_VDD_29_30;
			mmc->ocr_avail |= MMC_VDD_32_33;
			mmc->f_min = 300000;
			mmc->f_max = 50000000;
			mmc->caps =
				MMC_CAP_4_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED |
				MMC_CAP_SD_HIGHSPEED |
				MMC_CAP_SDIO_IRQ;
			mmc->max_seg_size = 0x80000;	/* 512KB */
			mmc->max_hw_segs = 1;
			mmc->max_phys_segs = 1;
			mmc->max_req_size = mmc->max_seg_size;
			mmc->max_blk_size = 512;
			mmc->max_blk_count = 0xffff;
			rval = mmc_add_host(mmc);
			if (rval < 0)
				goto done;
			}
		}

	/* Register vsdhc event handler */
	ipc_sdhc_event_register(boss_sdhc_event_handler, sdhc);

	/* Register vsdhc sdio interrupt handler */
	ipc_sdhc_sdio_irq_register(boss_sdhc_sdio_irq_handler, sdhc);

	rval = 0;

#if defined(CONFIG_PROC_FS)
	boss_sdhc_procfs_init(sdhc);
#endif

done:

	if (rval < 0) {
		ipc_sdhc_event_unregister(boss_sdhc_event_handler);
		if (sdhc) {
			for (i = 0; i < MAX_SDHC_HOST; i++)
				for (j = 0; j < MAX_SDHC_CARD_PER_HOST; j++)
					if (sdhc->mh[i][j]) {
						mmc_remove_host(sdhc->
								mh[i][j]->mmc);
						mmc_free_host(sdhc->
							      mh[i][j]->mmc);
			}
			kfree(sdhc);
		}
	}

	return 0;
}

/*
 * BOSS SDHC driver removal.
 */
static int __devexit boss_sdhc_remove(struct platform_device *pdev)
{
	struct boss_sdhc_s *sdhc =
		(struct boss_sdhc_s *) platform_get_drvdata(pdev);
	int i, j;

#if defined(CONFIG_PROC_FS)
	boss_sdhc_procfs_cleanup();
#endif

	for (i = 0; i < MAX_SDHC_HOST; i++)
		for (j = 0; j < MAX_SDHC_CARD_PER_HOST; j++)
			if (sdhc->mh[i][j]) {
				mmc_remove_host(sdhc->mh[i][j]->mmc);
				mmc_free_host(sdhc->mh[i][j]->mmc);
			}

	ipc_sdhc_event_unregister(boss_sdhc_event_handler);
	ipc_sdhc_sdio_irq_unregister(boss_sdhc_sdio_irq_handler);

	platform_set_drvdata(pdev, NULL);
	kfree(sdhc);

	return 0;
}

/*
 * Platform device object.
 */
static struct platform_device *boss_sdhc_device = NULL;

/*
 * Platform driver object.
 */
static struct platform_driver boss_sdhc_driver = {
	.probe =	boss_sdhc_probe,
	.remove =	boss_sdhc_remove,
	.driver		= {
		.name =		"boss_sdhc",
		.owner	= THIS_MODULE,
	},
};


/*
 * BOSS SDHC virtual driver initialization.
 */
static int __init boss_sdhc_init(void)
{
	int rval;

	boss_sdhc_device = platform_device_alloc("boss_sdhc", -1);
	if (boss_sdhc_device == NULL)
		return -ENOMEM;

	rval = platform_device_add(boss_sdhc_device);
	if (rval) {
		platform_device_put(boss_sdhc_device);
		return rval;
	}

	return platform_driver_register(&boss_sdhc_driver);
}

/*
 * BOSS SDHC virtual driver cleanup.
 */
static void __exit boss_sdhc_cleanup(void)
{
	platform_device_unregister(boss_sdhc_device);
	platform_driver_unregister(&boss_sdhc_driver);
}

module_init(boss_sdhc_init);
module_exit(boss_sdhc_cleanup);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charles Chiou <cchiou@ambarella.com>");
MODULE_DESCRIPTION("SDHC virtual driver under BOSS");
#endif
