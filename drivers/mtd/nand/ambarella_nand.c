/*
 * drivers/mtd/ambarella_nand.c
 *
 * History:
 *	2008/04/11 - [Cao Rongrong & Chien-Yang Chen] created file
 *	2009/01/04 - [Anthony Ginger] Port to 2.6.28
 *
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
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/dma.h>

#include <mach/hardware.h>
#include <mach/dma.h>
#include <plat/nand.h>
#include <plat/ptb.h>

#include <mach/boss.h>

#define AMBARELLA_NAND_DMA_BUFFER_SIZE	4096

/* nand_check_wp will be checked before write, so wait MTD fix */
#undef AMBARELLA_NAND_WP

#ifdef CONFIG_MTD_CMDLINE_PARTS
static const char *part_probes[] = { "cmdlinepart", NULL };
#endif

struct ambarella_nand_info {
	struct nand_chip			chip;
	struct mtd_info			mtd;
	struct nand_hw_control		controller;

	struct device			*dev;
	wait_queue_head_t		wq;

	struct ambarella_platform_nand	*plat_nand;
	int				suspend;
	int				dma_irq;
	int				cmd_irq;
	int				wp_gpio;
	unsigned char __iomem		*regbase;
	u32				dmabase;

	dma_addr_t			dmaaddr;
	u8				*dmabuf;
	int				dma_bufpos;
	u32				dma_status;
	u32				fio_dma_sta;
	atomic_t			irq_flag;

	/* saved column/page_addr during CMD_SEQIN */
	int				seqin_column;
	int				seqin_page_addr;

	/* Operation parameters for nand controller register */
	int				err_code;
	u32				cmd;
	u32				control_reg;
	u32				addr_hi;
	u32				addr;
	u32				dst;
	dma_addr_t			buf_phys;
	u32				len;
	u32				area;
	u32				ecc;

	u32				origin_clk;	/* in Khz */
	struct ambarella_nand_timing	*origin_timing;
	struct ambarella_nand_timing	current_timing;

	struct notifier_block		system_event;
	struct semaphore		system_event_sem;
};

static struct nand_ecclayout amb_oobinfo_512 = {
	.eccbytes = 6,
	.eccpos = {5, 6, 7, 8, 9, 10},
	.oobfree = {{0, 5}, {11, 5}}
};

static struct nand_ecclayout amb_oobinfo_2048 = {
	.eccbytes = 24,
	.eccpos = { 0, 8, 9, 10,11, 12,
		16, 24, 25, 26, 27, 28,
		32, 40, 41, 42, 43, 44,
		48, 56, 57, 58, 59, 60},
	.oobfree = {{1, 7}, {13, 3},
		{17, 7}, {29, 3},
		{33, 7}, {45, 3},
		{49, 7}, {61, 3}}
};

static uint8_t amb_scan_ff_pattern[] = { 0xff, };

static struct nand_bbt_descr amb_512_bbt_descr = {
	.offs = 5,
	.len = 1,
	.pattern = amb_scan_ff_pattern
};

static struct nand_bbt_descr amb_2048_bbt_descr = {
	.offs = 0,
	.len = 1,
	.pattern = amb_scan_ff_pattern
};


/*
 * The generic flash bbt decriptors overlap with our ecc
 * hardware, so define some Ambarella specific ones.
 */
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 1,
	.len = 4,
	.veroffs = 13,
	.maxblocks = 4,
	.pattern = bbt_pattern,
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 1,
	.len = 4,
	.veroffs = 13,
	.maxblocks = 4,
	.pattern = mirror_pattern,
};


/* ==========================================================================*/
#ifdef CONFIG_MTD_PARTITIONS
static char part_name[PART_MAX][PART_NAME_LEN];
#endif

/* ==========================================================================*/
#define NAND_TIMING_RSHIFT24BIT(x)	(((x) & 0xff000000) >> 24)
#define NAND_TIMING_RSHIFT16BIT(x)	(((x) & 0x00ff0000) >> 16)
#define NAND_TIMING_RSHIFT8BIT(x)	(((x) & 0x0000ff00) >> 8)
#define NAND_TIMING_RSHIFT0BIT(x)	(((x) & 0x000000ff) >> 0)

#define NAND_TIMING_LSHIFT24BIT(x)	((x) << 24)
#define NAND_TIMING_LSHIFT16BIT(x)	((x) << 16)
#define NAND_TIMING_LSHIFT8BIT(x)	((x) << 8)
#define NAND_TIMING_LSHIFT0BIT(x)	((x) << 0)

#define NAND_TIMING_CALC_SPDUP(val, new_clk, origin_clk)		\
	((val) * (new_clk) / (origin_clk))
#define NAND_TIMING_CALC_SPDDOWN(val, new_clk, origin_clk)	\
	((val) * (new_clk) / (origin_clk) +			\
	 (((val) * (new_clk)) % (origin_clk) ? 1 : 0))
#define NAND_TIMING_CALC_NEW(val, new_clk, origin_clk)		\
	((new_clk) > (origin_clk) ?				\
	NAND_TIMING_CALC_SPDUP(val, new_clk, origin_clk) :	\
	NAND_TIMING_CALC_SPDDOWN(val, new_clk, origin_clk))

static void amb_nand_set_timing(struct ambarella_nand_info *nand_info,
	struct ambarella_nand_timing *timing);

static u32 ambnand_calc_timing(struct ambarella_nand_info *nand_info, u32 idx)
{
	u32 origin_clk = nand_info->origin_clk;
	u32 new_clk = get_ahb_bus_freq_hz() / 1000;
	u32 origin_timing, timing_reg = 0;
	u8 tcls, tals, tcs, tds;
	u8 tclh, talh, tch, tdh;
	u8 twp, twh, twb, trr;
	u8 trp, treh, trb, tceh;
	u8 trdelay, tclr, twhr, tir;
	u8 tww, trhz, tar;

	switch (idx) {
	case 0: /* Calculate timing 0 */
		origin_timing = nand_info->origin_timing->timing0;
		if (origin_clk == new_clk)
			return origin_timing;

		tcls = NAND_TIMING_RSHIFT24BIT(origin_timing);
		tals = NAND_TIMING_RSHIFT16BIT(origin_timing);
		tcs = NAND_TIMING_RSHIFT8BIT(origin_timing);
		tds = NAND_TIMING_RSHIFT0BIT(origin_timing);

		tcls = NAND_TIMING_CALC_NEW(tcls, new_clk, origin_clk);
		tals = NAND_TIMING_CALC_NEW(tals, new_clk, origin_clk);
		tcs = NAND_TIMING_CALC_NEW(tcs, new_clk, origin_clk);
		tds = NAND_TIMING_CALC_NEW(tds, new_clk, origin_clk);

		timing_reg = NAND_TIMING_LSHIFT24BIT(tcls) |
			NAND_TIMING_LSHIFT16BIT(tals) |
			NAND_TIMING_LSHIFT8BIT(tcs) |
			NAND_TIMING_LSHIFT0BIT(tds);
		break;
	case 1: /* Calculate timing 1*/
		origin_timing = nand_info->origin_timing->timing1;
		if (origin_clk == new_clk)
			return origin_timing;

		tclh = NAND_TIMING_RSHIFT24BIT(origin_timing);
		talh = NAND_TIMING_RSHIFT16BIT(origin_timing);
		tch = NAND_TIMING_RSHIFT8BIT(origin_timing);
		tdh = NAND_TIMING_RSHIFT0BIT(origin_timing);

		tclh = NAND_TIMING_CALC_NEW(tclh, new_clk, origin_clk);
		talh = NAND_TIMING_CALC_NEW(talh, new_clk, origin_clk);
		tch = NAND_TIMING_CALC_NEW(tch, new_clk, origin_clk);
		tdh = NAND_TIMING_CALC_NEW(tdh, new_clk, origin_clk);

		timing_reg = NAND_TIMING_LSHIFT24BIT(tclh) |
			NAND_TIMING_LSHIFT16BIT(talh) |
			NAND_TIMING_LSHIFT8BIT(tch) |
			NAND_TIMING_LSHIFT0BIT(tdh);
		break;
	case 2: /* Calculate timing 2*/
		origin_timing = nand_info->origin_timing->timing2;
		if (origin_clk == new_clk)
			return origin_timing;

		twp = NAND_TIMING_RSHIFT24BIT(origin_timing);
		twh = NAND_TIMING_RSHIFT16BIT(origin_timing);
		twb = NAND_TIMING_RSHIFT8BIT(origin_timing) + 1;
		trr = NAND_TIMING_RSHIFT0BIT(origin_timing);

		twp = NAND_TIMING_CALC_NEW(twp, new_clk, origin_clk);
		twh = NAND_TIMING_CALC_NEW(twh, new_clk, origin_clk);
		twb = NAND_TIMING_CALC_NEW(twb, new_clk, origin_clk);
		twb = (twb - 1) ? (twb - 1) : 1;
		trr = NAND_TIMING_CALC_NEW(trr, new_clk, origin_clk);

		timing_reg = NAND_TIMING_LSHIFT24BIT(twp) |
			NAND_TIMING_LSHIFT16BIT(twh) |
			NAND_TIMING_LSHIFT8BIT(twb) |
			NAND_TIMING_LSHIFT0BIT(trr);
		break;
	case 3: /* Calculate timing 3*/
		origin_timing = nand_info->origin_timing->timing3;
		if (origin_clk == new_clk)
			return origin_timing;

		trp = NAND_TIMING_RSHIFT24BIT(origin_timing);
		treh = NAND_TIMING_RSHIFT16BIT(origin_timing);
		trb = NAND_TIMING_RSHIFT8BIT(origin_timing) + 1;
		tceh = NAND_TIMING_RSHIFT0BIT(origin_timing) + 1;

		trp = NAND_TIMING_CALC_NEW(trp, new_clk, origin_clk);
		treh = NAND_TIMING_CALC_NEW(treh, new_clk, origin_clk);
		trb = NAND_TIMING_CALC_NEW(trb, new_clk, origin_clk);
		trb = (trb - 1) ? (trb - 1) : 1;
		tceh = NAND_TIMING_CALC_NEW(tceh, new_clk, origin_clk);
		tceh = (tceh - 1) ? (tceh - 1) : 1;

		timing_reg = NAND_TIMING_LSHIFT24BIT(trp) |
			NAND_TIMING_LSHIFT16BIT(treh) |
			NAND_TIMING_LSHIFT8BIT(trb) |
			NAND_TIMING_LSHIFT0BIT(tceh);
		break;
	case 4: /* Calculate timing 4*/
		origin_timing = nand_info->origin_timing->timing4;
		if (origin_clk == new_clk)
			return origin_timing;

		trdelay = NAND_TIMING_RSHIFT24BIT(origin_timing) + 1;
		tclr = NAND_TIMING_RSHIFT16BIT(origin_timing);
		twhr = NAND_TIMING_RSHIFT8BIT(origin_timing);
		tir = NAND_TIMING_RSHIFT0BIT(origin_timing);

		trdelay = NAND_TIMING_CALC_NEW(trdelay, new_clk, origin_clk);
		trdelay = (trdelay - 1) ? (trdelay - 1) : 1;
		tclr = NAND_TIMING_CALC_NEW(tclr, new_clk, origin_clk);
		twhr = NAND_TIMING_CALC_NEW(twhr, new_clk, origin_clk);
		tir = NAND_TIMING_CALC_NEW(tir, new_clk, origin_clk);

		timing_reg = NAND_TIMING_LSHIFT24BIT(trdelay) |
			NAND_TIMING_LSHIFT16BIT(tclr) |
			NAND_TIMING_LSHIFT8BIT(twhr) |
			NAND_TIMING_LSHIFT0BIT(tir);
		break;
	case 5: /* Calculate timing 5*/
		origin_timing = nand_info->origin_timing->timing5;
		if (origin_clk == new_clk)
			return origin_timing;

		tww = NAND_TIMING_RSHIFT16BIT(origin_timing);
		trhz = NAND_TIMING_RSHIFT8BIT(origin_timing) + 1;
		tar = NAND_TIMING_RSHIFT0BIT(origin_timing) + 1;

		tww = NAND_TIMING_CALC_NEW(tww, new_clk, origin_clk);
		trhz = NAND_TIMING_CALC_NEW(trhz, new_clk, origin_clk);
		trhz = (trhz - 1) ? (trhz - 1) : 1;
		tar = NAND_TIMING_CALC_NEW(tar, new_clk, origin_clk);
		tar = (tar - 1) ? (tar - 1) : 1;

		timing_reg = NAND_TIMING_LSHIFT16BIT(tww) |
			NAND_TIMING_LSHIFT8BIT(trhz) |
			NAND_TIMING_LSHIFT0BIT(tar);
		break;
	}

	return timing_reg;
}

static int ambarella_nand_system_event(struct notifier_block *nb,
	unsigned long val, void *data)
{
	int					errorCode = NOTIFY_OK;
	struct ambarella_nand_info		*nand_info;

	nand_info = container_of(nb, struct ambarella_nand_info, system_event);

	switch (val) {
	case AMBA_EVENT_PRE_CPUFREQ:
		pr_debug("%s: Pre Change\n", __func__);
		down(&nand_info->system_event_sem);
		break;

	case AMBA_EVENT_POST_CPUFREQ:
		pr_debug("%s: Post Change\n", __func__);
		/* The timming register is default value,
		 * it's big enough to operate
		 * with NAND, so no need to change it. */
		if (nand_info->origin_timing->control != 0) {
			nand_info->current_timing.timing0 = ambnand_calc_timing(nand_info, 0);
			nand_info->current_timing.timing1 = ambnand_calc_timing(nand_info, 1);
			nand_info->current_timing.timing2 = ambnand_calc_timing(nand_info, 2);
			nand_info->current_timing.timing3 = ambnand_calc_timing(nand_info, 3);
			nand_info->current_timing.timing4 = ambnand_calc_timing(nand_info, 4);
			nand_info->current_timing.timing5 = ambnand_calc_timing(nand_info, 5);
			amb_nand_set_timing(nand_info, &nand_info->current_timing);

			pr_debug("origin reg:\t0x%08x 0x%08x"
				" 0x%08x 0x%08x 0x%08x 0x%08x\n",
				nand_info->origin_timing->timing0,
				nand_info->origin_timing->timing1,
				nand_info->origin_timing->timing2,
				nand_info->origin_timing->timing3,
				nand_info->origin_timing->timing4,
				nand_info->origin_timing->timing5);

			pr_debug("new reg:\t0x%08x 0x%08x"
				" 0x%08x 0x%08x 0x%08x 0x%08x\n",
				nand_info->current_timing.timing0,
				nand_info->current_timing.timing1,
				nand_info->current_timing.timing2,
				nand_info->current_timing.timing3,
				nand_info->current_timing.timing4,
				nand_info->current_timing.timing5);
		}
		up(&nand_info->system_event_sem);
		break;

	default:
		break;
	}

	return errorCode;
}

static irqreturn_t nand_fiocmd_isr_handler(int irq, void *dev_id)
{
	struct ambarella_nand_info		*nand_info;
	u32					val;

	nand_info = (struct ambarella_nand_info *)dev_id;

	val = amba_readl(nand_info->regbase + FIO_STA_OFFSET);

	if (val & FIO_STA_FI) {
		amba_clrbitsl(nand_info->regbase + FIO_CTR_OFFSET,
			(FIO_CTR_RS | FIO_CTR_SE | FIO_CTR_CO));

		amba_writel(nand_info->regbase + FLASH_INT_OFFSET, 0x0);

		atomic_clear_mask(0x1, (unsigned long *)&nand_info->irq_flag);
		wake_up(&nand_info->wq);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static irqreturn_t nand_fiodma_isr_handler(int irq, void *dev_id)
{
	struct ambarella_nand_info		*nand_info;
	u32					val;

	nand_info = (struct ambarella_nand_info *)dev_id;

	val = amba_readl(nand_info->regbase + FIO_DMACTR_OFFSET);

	if ((val & (FIO_DMACTR_SD | FIO_DMACTR_CF |
		FIO_DMACTR_XD | FIO_DMACTR_FL)) ==  FIO_DMACTR_FL) {
		nand_info->fio_dma_sta =
			amba_readl(nand_info->regbase + FIO_DMASTA_OFFSET);

		amba_writel(nand_info->regbase + FIO_DMASTA_OFFSET, 0x0);

		atomic_clear_mask(0x2, (unsigned long *)&nand_info->irq_flag);
		wake_up(&nand_info->wq);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static void nand_dma_isr_handler(void *dev_id, u32 dma_status)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)dev_id;

#if defined(CONFIG_AMBARELLA_IPC)
	if (nand_info->cmd == 0)
		return;
#endif
	nand_info->dma_status = dma_status;
	atomic_clear_mask(0x4, (unsigned long *)&nand_info->irq_flag);
	wake_up(&nand_info->wq);
}

#if defined(CONFIG_AMBARELLA_IPC)
#define VIC2_INT_VEC_OFFSET	32
#define VIC3_INT_VEC_OFFSET	64
static inline void ambarella_nand_vic_ctrl(u32 line, u32 enable)
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
#if  (VIC_INSTANCES >= 3)
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

static void nand_amb_setup_dma_devmem(struct ambarella_nand_info *nand_info)
{
	u32					val;
	ambarella_dma_req_t			dma_req;

	dma_req.src = nand_info->dmabase;
	dma_req.dst = nand_info->buf_phys;
	dma_req.next = NULL;
	dma_req.rpt = (u32) NULL;
	dma_req.xfr_count = nand_info->len;
	if (nand_info->len > 16) {
		dma_req.attr = DMA_CHANX_CTR_WM |
			DMA_CHANX_CTR_NI |
			DMA_NODC_MN_BURST_SIZE;
	} else {
		dma_req.attr = DMA_CHANX_CTR_WM |
			DMA_CHANX_CTR_NI |
			DMA_NODC_SP_BURST_SIZE;
	}

	ambarella_dma_xfr(&dma_req, FIO_DMA_CHAN);

	amba_writel(nand_info->regbase + FIO_DMAADR_OFFSET, nand_info->addr);

	if (nand_info->len > 16) {
		val = FIO_DMACTR_EN |
			FIO_DMACTR_FL |
			FIO_MN_BURST_SIZE |
			nand_info->len;
	} else {
		val = FIO_DMACTR_EN |
			FIO_DMACTR_FL |
			FIO_SP_BURST_SIZE |
			nand_info->len;
	}
	amba_writel(nand_info->regbase + FIO_DMACTR_OFFSET, val);
}

static void nand_amb_setup_dma_memdev(struct ambarella_nand_info *nand_info)
{
	u32					val;
	ambarella_dma_req_t			dma_req;

	dma_req.src = nand_info->buf_phys;
	dma_req.dst = nand_info->dmabase;
	dma_req.next = NULL;
	dma_req.rpt = (u32) NULL;
	dma_req.xfr_count = nand_info->len;
	if (nand_info->len > 16) {
		dma_req.attr = DMA_CHANX_CTR_RM |
			DMA_CHANX_CTR_NI |
			DMA_NODC_MN_BURST_SIZE;
	} else {
		dma_req.attr = DMA_CHANX_CTR_RM |
			DMA_CHANX_CTR_NI |
			DMA_NODC_SP_BURST_SIZE;
	}

	ambarella_dma_xfr(&dma_req, FIO_DMA_CHAN);

	amba_writel(nand_info->regbase + FIO_DMAADR_OFFSET, nand_info->addr);

	if (nand_info->len > 16) {
		val = FIO_DMACTR_EN |
			FIO_DMACTR_FL |
			FIO_MN_BURST_SIZE |
			FIO_DMACTR_RM |
			nand_info->len;
	} else {
		val = FIO_DMACTR_EN |
			FIO_DMACTR_FL |
			FIO_SP_BURST_SIZE |
			FIO_DMACTR_RM |
			nand_info->len;
	}
	amba_writel(nand_info->regbase + FIO_DMACTR_OFFSET, val);
}

static int nand_amb_request(struct ambarella_nand_info *nand_info)
{
	int					errorCode = 0;
	u32					cmd;
	u32					nand_ctr_reg = 0;
	u32					nand_cmd_reg = 0;
	u32					fio_ctr_reg = 0;
	long					timeout;

	if (unlikely(nand_info->suspend == 1)) {
		dev_err(nand_info->dev, "%s: suspend!\n", __func__);
		errorCode = -EPERM;
		goto nand_amb_request_exit;
	}

	cmd = nand_info->cmd;

	nand_ctr_reg = nand_info->control_reg | NAND_CTR_WAS;

	nand_info->plat_nand->request();

#if defined(CONFIG_AMBARELLA_IPC)
	boss_set_irq_owner(DMA_FIOS_IRQ, BOSS_IRQ_OWNER_LINUX, 1);
	boss_set_irq_owner(nand_info->cmd_irq, BOSS_IRQ_OWNER_LINUX, 1);
	boss_set_irq_owner(nand_info->dma_irq, BOSS_IRQ_OWNER_LINUX, 1);
#if (CHIP_REV == I1)
	enable_irq(DMA_FIOS_IRQ);
	enable_irq(nand_info->cmd_irq);
	enable_irq(nand_info->dma_irq);
#endif
#endif

#ifdef AMBARELLA_NAND_WP
	if ((cmd == NAND_AMB_CMD_ERASE ||
		cmd == NAND_AMB_CMD_COPYBACK ||
		cmd == NAND_AMB_CMD_PROGRAM) &&
		nand_info->wp_gpio >= 0)
		gpio_direction_output(nand_info->wp_gpio, GPIO_HIGH);
#endif

	switch (cmd) {
	case NAND_AMB_CMD_RESET:
		nand_cmd_reg = NAND_AMB_CMD_RESET;
		amba_writel(nand_info->regbase + FLASH_CMD_OFFSET,
			nand_cmd_reg);
		break;

	case NAND_AMB_CMD_READID:
		nand_ctr_reg |= NAND_CTR_A(nand_info->addr_hi);
		nand_cmd_reg = nand_info->addr | NAND_AMB_CMD_READID;
		amba_writel(nand_info->regbase + FLASH_CTR_OFFSET,
			nand_ctr_reg);
		amba_writel(nand_info->regbase + FLASH_CMD_OFFSET,
			nand_cmd_reg);
		break;

	case NAND_AMB_CMD_READSTATUS:
		nand_ctr_reg |= NAND_CTR_A(nand_info->addr_hi);
		nand_cmd_reg = nand_info->addr | NAND_AMB_CMD_READSTATUS;
		amba_writel(nand_info->regbase + FLASH_CTR_OFFSET,
			nand_ctr_reg);
		amba_writel(nand_info->regbase + FLASH_CMD_OFFSET,
			nand_cmd_reg);
		break;

	case NAND_AMB_CMD_ERASE:
		nand_ctr_reg |= NAND_CTR_A(nand_info->addr_hi);
		nand_cmd_reg = nand_info->addr | NAND_AMB_CMD_ERASE;
		amba_writel(nand_info->regbase + FLASH_CTR_OFFSET,
			nand_ctr_reg);
		amba_writel(nand_info->regbase + FLASH_CMD_OFFSET,
			nand_cmd_reg);
		break;

	case NAND_AMB_CMD_COPYBACK:
		nand_ctr_reg |= NAND_CTR_A(nand_info->addr_hi);
		nand_ctr_reg |= NAND_CTR_CE;
		nand_cmd_reg = nand_info->addr | NAND_AMB_CMD_COPYBACK;
		amba_writel(nand_info->regbase + FLASH_CFI_OFFSET,
			nand_info->dst);
		amba_writel(nand_info->regbase + FLASH_CTR_OFFSET,
			nand_ctr_reg);
		amba_writel(nand_info->regbase + FLASH_CMD_OFFSET,
			nand_cmd_reg);
		break;

	case NAND_AMB_CMD_READ:
		nand_ctr_reg |= NAND_CTR_A(nand_info->addr_hi);

		if (nand_info->area == MAIN_ECC)
			nand_ctr_reg |= (NAND_CTR_SE);
		else if (nand_info->area == SPARE_ONLY ||
			nand_info->area == SPARE_ECC)
			nand_ctr_reg |= (NAND_CTR_SE | NAND_CTR_SA);

		fio_ctr_reg = amba_readl(nand_info->regbase + FIO_CTR_OFFSET);

		if (nand_info->area == SPARE_ONLY ||
			nand_info->area == SPARE_ECC  ||
			nand_info->area == MAIN_ECC)
			fio_ctr_reg |= (FIO_CTR_RS);
#ifdef CONFIG_MTD_NAND_AMBARELLA_SOFT_ECC
		nand_info->ecc = EC_MDSD;
#endif
		switch (nand_info->ecc) {
		case EC_MDSE:
			nand_ctr_reg |= NAND_CTR_EC_SPARE;
			fio_ctr_reg |= FIO_CTR_CO;
			break;
		case EC_MESD:
			nand_ctr_reg |= NAND_CTR_EC_MAIN;
			fio_ctr_reg |= FIO_CTR_CO;
			break;
		case EC_MESE:
			nand_ctr_reg |=	(NAND_CTR_EC_MAIN | NAND_CTR_EC_SPARE);
			fio_ctr_reg |= FIO_CTR_CO;
			break;
		case EC_MDSD:
		default:
			break;
		}

		amba_writel(nand_info->regbase + FIO_CTR_OFFSET,
			fio_ctr_reg);
		amba_writel(nand_info->regbase + FLASH_CTR_OFFSET,
			nand_ctr_reg);
		nand_amb_setup_dma_devmem(nand_info);

		break;

	case NAND_AMB_CMD_PROGRAM:
		nand_ctr_reg |= NAND_CTR_A(nand_info->addr_hi);

		if (nand_info->area == MAIN_ECC)
			nand_ctr_reg |= (NAND_CTR_SE);
		else if (nand_info->area == SPARE_ONLY ||
			nand_info->area == SPARE_ECC)
			nand_ctr_reg |= (NAND_CTR_SE | NAND_CTR_SA);

		fio_ctr_reg = amba_readl(nand_info->regbase + FIO_CTR_OFFSET);

		if (nand_info->area == SPARE_ONLY ||
			nand_info->area == SPARE_ECC  ||
			nand_info->area == MAIN_ECC)
			fio_ctr_reg |= (FIO_CTR_RS);

#ifdef CONFIG_MTD_NAND_AMBARELLA_SOFT_ECC
		nand_info->ecc = EC_MDSD;
#endif
		switch (nand_info->ecc) {
		case EG_MDSE :
			nand_ctr_reg |= NAND_CTR_EG_SPARE;
			break;
		case EG_MESD :
			nand_ctr_reg |= NAND_CTR_EG_MAIN;
			break;
		case EG_MESE :
			nand_ctr_reg |= (NAND_CTR_EG_MAIN | NAND_CTR_EG_SPARE);
			break;
		case EG_MDSD:
		default:
			break;
		}

		amba_writel(nand_info->regbase + FIO_CTR_OFFSET,
			fio_ctr_reg);
		amba_writel(nand_info->regbase + FLASH_CTR_OFFSET,
			nand_ctr_reg);
		nand_amb_setup_dma_memdev(nand_info);

		break;

	default:
		dev_warn(nand_info->dev,
			"%s: wrong command %d!\n", __func__, cmd);
		errorCode = -EINVAL;
		goto nand_amb_request_done;
		break;
	}

	if (cmd == NAND_AMB_CMD_READ || cmd == NAND_AMB_CMD_PROGRAM) {
		timeout = wait_event_timeout(nand_info->wq,
			atomic_read(&nand_info->irq_flag) == 0x0, 1 * HZ);
		if (timeout <= 0) {
			errorCode = -EBUSY;
			dev_err(nand_info->dev, "%s: cmd=0x%x timeout 0x%08x\n",
				__func__, cmd, atomic_read(&nand_info->irq_flag));
		} else {
			dev_dbg(nand_info->dev, "%ld jiffies left.\n", timeout);
		}

		if (nand_info->dma_status & (DMA_CHANX_STA_OE | DMA_CHANX_STA_ME |
			DMA_CHANX_STA_BE | DMA_CHANX_STA_RWE |
			DMA_CHANX_STA_AE)) {
			dev_err(nand_info->dev,
				"%s: Errors happend in DMA transaction %d!\n",
				__func__, nand_info->dma_status);
			errorCode = -EIO;
			goto nand_amb_request_done;
		}

		errorCode = nand_info->plat_nand->parse_error(
			nand_info->fio_dma_sta);
		if (errorCode) {
			u32 block_addr;
			block_addr = nand_info->addr /
					nand_info->mtd.erasesize *
					nand_info->mtd.erasesize;
			dev_err(nand_info->dev,
				"%s: cmd=0x%x, addr_hi=0x%x, "
				"addr=0x%x, dst=0x%x, buf=0x%x, "
				"len=0x%x, area=0x%x, ecc=0x%x, "
				"block addr=0x%x!\n",
				__func__, cmd,
				nand_info->addr_hi,
				nand_info->addr,
				nand_info->dst,
				nand_info->buf_phys,
				nand_info->len,
				nand_info->area,
				nand_info->ecc,
				block_addr);
			goto nand_amb_request_done;
		}
	} else {
		/* just wait cmd irq, no care about both DMA irqs */
		timeout = wait_event_timeout(nand_info->wq,
			(atomic_read(&nand_info->irq_flag) & 0x1) == 0x0, 1 * HZ);
		if (timeout <= 0) {
			errorCode = -EBUSY;
			dev_err(nand_info->dev, "%s: cmd=0x%x timeout 0x%08x\n",
				__func__, cmd, atomic_read(&nand_info->irq_flag));
			goto nand_amb_request_done;
		} else {
			dev_dbg(nand_info->dev, "%ld jiffies left.\n", timeout);

			if (cmd == NAND_AMB_CMD_READID) {
				u32 id = amba_readl(nand_info->regbase +
					FLASH_ID_OFFSET);
				nand_info->dmabuf[0] = (unsigned char) (id >> 24);
				nand_info->dmabuf[1] = (unsigned char) (id >> 16);
				nand_info->dmabuf[2] = (unsigned char) (id >> 8);
				nand_info->dmabuf[3] = (unsigned char) id;
			} else if (cmd == NAND_AMB_CMD_READSTATUS) {
				*nand_info->dmabuf = amba_readl(nand_info->regbase +
					FLASH_STA_OFFSET);
			}
		}
	}

nand_amb_request_done:
	atomic_set(&nand_info->irq_flag, 0x7);
	nand_info->dma_status = 0;
	/* Avoid to flush previous error info */
	if (nand_info->err_code == 0)
		nand_info->err_code = errorCode;

#ifdef AMBARELLA_NAND_WP
	if ((cmd == NAND_AMB_CMD_ERASE ||
		cmd == NAND_AMB_CMD_COPYBACK ||
		cmd == NAND_AMB_CMD_PROGRAM) &&
		nand_info->wp_gpio >= 0)
		gpio_direction_output(nand_info->wp_gpio, GPIO_LOW);
#endif

#if defined(CONFIG_AMBARELLA_IPC)
	nand_info->cmd = 0;
#if (CHIP_REV == I1)
	disable_irq(DMA_FIOS_IRQ);
	disable_irq(nand_info->cmd_irq);
	disable_irq(nand_info->dma_irq);
#endif
#endif

	nand_info->plat_nand->release();

nand_amb_request_exit:
	return errorCode;
}

int nand_amb_reset(struct ambarella_nand_info *nand_info)
{
	nand_info->cmd = NAND_AMB_CMD_RESET;

	return nand_amb_request(nand_info);
}

int nand_amb_read_id(struct ambarella_nand_info *nand_info)
{
	nand_info->cmd = NAND_AMB_CMD_READID;
	nand_info->addr_hi = 0;
	nand_info->addr = 0;

	return nand_amb_request(nand_info);
}

int nand_amb_read_status(struct ambarella_nand_info *nand_info)
{
	nand_info->cmd = NAND_AMB_CMD_READSTATUS;
	nand_info->addr_hi = 0;
	nand_info->addr = 0;

	return nand_amb_request(nand_info);
}

int nand_amb_erase(struct ambarella_nand_info *nand_info, u32 page_addr)
{
	int					errorCode = 0;
	u32					addr_hi;
	u32					addr;
	u64					addr64;

	addr64 = (u64)(page_addr * nand_info->mtd.writesize);
	addr_hi = (u32)(addr64 >> 32);
	addr = (u32)addr64;

	nand_info->cmd = NAND_AMB_CMD_ERASE;
	nand_info->addr_hi = addr_hi;
	nand_info->addr = addr;

	errorCode = nand_amb_request(nand_info);

	return errorCode;
}

int nand_amb_read_data(struct ambarella_nand_info *nand_info,
	u32 page_addr, dma_addr_t buf_dma, u8 area)
{
	int					errorCode = 0;
	u32					addr_hi;
	u32					addr;
	u32					len;
	u64					addr64;
	u8					ecc;

	addr64 = (u64)(page_addr * nand_info->mtd.writesize);
	addr_hi = (u32)(addr64 >> 32);
	addr = (u32)addr64;

	switch (area) {
	case MAIN_ONLY:
		ecc = EC_MDSD;
		len = nand_info->mtd.writesize;
		break;
	case MAIN_ECC:
		ecc = EC_MESD;
		len = nand_info->mtd.writesize;
		break;
	case SPARE_ONLY:
		ecc = EC_MDSD;
		len = nand_info->mtd.oobsize;
		break;
	case SPARE_ECC:
		ecc = EC_MDSE;
		len = nand_info->mtd.oobsize;
		break;
	default:
		dev_err(nand_info->dev, "%s: Wrong area.\n", __func__);
		errorCode = -EINVAL;
		goto nand_amb_read_page_exit;
		break;
	}

	nand_info->cmd = NAND_AMB_CMD_READ;
	nand_info->addr_hi = addr_hi;
	nand_info->addr = addr;
	nand_info->buf_phys = buf_dma;
	nand_info->len = len;
	nand_info->area = area;
	nand_info->ecc = ecc;

	errorCode = nand_amb_request(nand_info);

nand_amb_read_page_exit:
	return errorCode;
}

int nand_amb_write_data(struct ambarella_nand_info *nand_info,
	u32 page_addr, dma_addr_t buf_dma, u8 area)
{
	int					errorCode = 0;
	u32					addr_hi;
	u32					addr;
	u32					len;
	u64					addr64;
	u8					ecc;

	addr64 = (u64)(page_addr * nand_info->mtd.writesize);
	addr_hi = (u32)(addr64 >> 32);
	addr = (u32)addr64;

	switch (area) {
	case MAIN_ONLY:
		ecc = EG_MDSD;
		len = nand_info->mtd.writesize;
		break;
	case MAIN_ECC:
		ecc = EG_MESD;
		len = nand_info->mtd.writesize;
		break;
	case SPARE_ONLY:
		ecc = EG_MDSD;
		len = nand_info->mtd.oobsize;
		break;
	case SPARE_ECC:
		ecc = EG_MDSE;
		len = nand_info->mtd.oobsize;
		break;
	default:
		dev_err(nand_info->dev, "%s: Wrong area.\n", __func__);
		errorCode = -EINVAL;
		goto nand_amb_write_page_exit;
		break;
	}

	nand_info->cmd = NAND_AMB_CMD_PROGRAM;
	nand_info->addr_hi = addr_hi;
	nand_info->addr = addr;
	nand_info->buf_phys = buf_dma;
	nand_info->len = len;
	nand_info->area = area;
	nand_info->ecc = ecc;

	errorCode = nand_amb_request(nand_info);

nand_amb_write_page_exit:
	return errorCode;
}


/* ==========================================================================*/
static uint8_t amb_nand_read_byte(struct mtd_info *mtd)
{
	struct ambarella_nand_info		*nand_info;
	uint8_t					*data;

	nand_info = (struct ambarella_nand_info *)mtd->priv;

	data = nand_info->dmabuf + nand_info->dma_bufpos;
	nand_info->dma_bufpos++;

	return *data;
}

static u16 amb_nand_read_word(struct mtd_info *mtd)
{
	struct ambarella_nand_info		*nand_info;
	u16					*data;

	nand_info = (struct ambarella_nand_info *)mtd->priv;

	data = (u16 *)(nand_info->dmabuf + nand_info->dma_bufpos);
	nand_info->dma_bufpos += 2;

	return *data;
}

static void amb_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;

	if ((nand_info->dma_bufpos >= AMBARELLA_NAND_DMA_BUFFER_SIZE) ||
		((nand_info->dma_bufpos + len) > AMBARELLA_NAND_DMA_BUFFER_SIZE))
		BUG();

	memcpy(buf, nand_info->dmabuf + nand_info->dma_bufpos, len);
	nand_info->dma_bufpos += len;
}

static void amb_nand_write_buf(struct mtd_info *mtd,
	const uint8_t *buf, int len)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;

	if ((nand_info->dma_bufpos >= AMBARELLA_NAND_DMA_BUFFER_SIZE) ||
		((nand_info->dma_bufpos + len) > AMBARELLA_NAND_DMA_BUFFER_SIZE))
		BUG();

	memcpy(nand_info->dmabuf + nand_info->dma_bufpos, buf, len);
	nand_info->dma_bufpos += len;
}

static int amb_nand_verify_buf(struct mtd_info *mtd,
	const uint8_t *buf, int len)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;
	dev_info(nand_info->dev,
		"%s: We don't implement the verify function\n", __func__);

	return 0;
}

static void amb_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;

	if (chip > 0) {
		dev_err(nand_info->dev,
			"%s: Multi-Chip isn't supported yet.\n", __func__);
	}
}

static void amb_nand_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{

}

static int amb_nand_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip 			*chip = mtd->priv;

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	return (chip->read_byte(mtd) & NAND_STATUS_READY) ? 1 : 0;
}

static int amb_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
	int					status = 0;
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;

	/* ambarella nand controller has waited for the command completion,
	  * but still need to check the nand chip's status
	  */
	if (nand_info->err_code)
		status = NAND_STATUS_FAIL;
	else {
		chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
		status = chip->read_byte(mtd);
	}

	return status;
}

static void amb_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
	int column, int page_addr)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;
	nand_info->err_code = 0;

	switch(command) {
	case NAND_CMD_RESET:
		nand_amb_reset(nand_info);
		break;
	case NAND_CMD_READID:
		nand_info->dma_bufpos = 0;
		nand_amb_read_id(nand_info);
		break;
	case NAND_CMD_STATUS:
		nand_info->dma_bufpos = 0;
		nand_amb_read_status(nand_info);
		break;
	case NAND_CMD_ERASE1:
		nand_amb_erase(nand_info, page_addr);
		break;
	case NAND_CMD_ERASE2:
		break;
	case NAND_CMD_READOOB:
		nand_info->dma_bufpos = column;
		nand_amb_read_data(nand_info, page_addr,
			nand_info->dmaaddr, SPARE_ONLY);
		break;
	case NAND_CMD_READ0:
		nand_info->dma_bufpos = column;
		nand_amb_read_data(nand_info, page_addr,
			nand_info->dmaaddr, MAIN_ECC);
		nand_amb_read_data(nand_info, page_addr,
			nand_info->dmaaddr + mtd->writesize, SPARE_ONLY);
		break;
	case NAND_CMD_SEQIN:
		nand_info->dma_bufpos = column;
		nand_info->seqin_column = column;
		nand_info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		if (nand_info->seqin_column < mtd->writesize) {
			nand_amb_write_data(nand_info,
				nand_info->seqin_page_addr,
				nand_info->dmaaddr, MAIN_ECC);
		}
		nand_amb_write_data(nand_info,
				nand_info->seqin_page_addr,
				nand_info->dmaaddr + mtd->writesize,
				SPARE_ONLY);
		break;
	default:
		dev_err(nand_info->dev, "%s: 0x%x, %d, %d\n",
				__func__, command, column, page_addr);
		BUG();
		break;
	}
}

static void amb_nand_hwctl(struct mtd_info *mtd, int mode)
{
}

static int amb_nand_caculate_ecc(struct mtd_info *mtd,
	const u_char *dat, u_char *ecc_code)
{
	ecc_code[0] = 0xff;
	ecc_code[1] = 0xff;
	ecc_code[2] = 0xff;
	ecc_code[3] = 0xff;
	ecc_code[4] = 0xff;
	ecc_code[5] = 0xff;

	return 0;
}

static int amb_nand_correct_data(struct mtd_info *mtd, u_char *dat,
	u_char *read_ecc, u_char *calc_ecc)
{
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)mtd->priv;
	/*
	 * Any error include DMA error and FIO DMA error, we consider
	 * it as a ecc error which will tell the caller the read fail.
	 * We have distinguish all the errors, but the nand_read_ecc only
	 * check the return value by this function.
	 */
	return nand_info->err_code;
}

static int amb_nand_write_oob_std(struct mtd_info *mtd,
	struct nand_chip *chip, int page)
{
	int					i, status;

	/* Our nand controller will write the generated ECC code into spare
	  * area automatically, so we should mark the ECC code which located
	  * in the eccpos.
	  */
	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[chip->ecc.layout->eccpos[i]] = 0xFF;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/* ==========================================================================*/
static void amb_nand_set_timing(struct ambarella_nand_info *nand_info,
	struct ambarella_nand_timing *timing)
{
#ifndef CONFIG_MTD_NAND_AMBARELLA_DEFAULT_TIMINGS
	amba_writel(nand_info->regbase + FLASH_TIM0_OFFSET, timing->timing0);
	amba_writel(nand_info->regbase + FLASH_TIM1_OFFSET, timing->timing1);
	amba_writel(nand_info->regbase + FLASH_TIM2_OFFSET, timing->timing2);
	amba_writel(nand_info->regbase + FLASH_TIM3_OFFSET, timing->timing3);
	amba_writel(nand_info->regbase + FLASH_TIM4_OFFSET, timing->timing4);
	amba_writel(nand_info->regbase + FLASH_TIM5_OFFSET, timing->timing5);
#endif
}

static int __devinit ambarella_nand_config_flash(
	struct ambarella_nand_info *nand_info)
{
	int					errorCode = 0;

	amb_nand_set_timing(nand_info, nand_info->plat_nand->timing);

	/* control_reg will be uesd when real operation to NAND is performed */

	/* Calculate row address cycyle according to whether the page number
	  * of the nand is greater than 65536 */
	if ((nand_info->chip.chip_shift - nand_info->chip.page_shift) > 16)
		nand_info->control_reg |= NAND_CTR_P3;
	else
		nand_info->control_reg &= ~NAND_CTR_P3;

	nand_info->control_reg &= ~NAND_CTR_SZ_8G;
	switch (nand_info->chip.chipsize) {
	case 8 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_64M;
		break;
	case 16 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_128M;
		break;
	case 32 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_256M;
		break;
	case 64 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_512M;
		break;
	case 128 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_1G;
		break;
	case 256 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_2G;
		break;
	case 512 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_4G;
		break;
	case 1024 * 1024 * 1024:
		nand_info->control_reg |= NAND_CTR_SZ_8G;
		break;
	default:
		dev_err(nand_info->dev,
			"Unexpected NAND flash chipsize %lld. Aborting\n",
			nand_info->chip.chipsize);
		errorCode = -ENXIO;
		break;
	}

	printk("%s: 0x%08x, 0x%08x\n", __func__,
		nand_info->plat_nand->timing->control, nand_info->control_reg);

	return errorCode;
}

static int __devinit ambarella_nand_init_chip(struct ambarella_nand_info *nand_info)
{
	u32 sys_config, cfg_page_size, cfg_read_confirm;
	struct nand_chip *chip = &nand_info->chip;

	sys_config = amba_readl(SYS_CONFIG_REG);

	cfg_page_size = sys_config & SYS_CONFIG_NAND_FLASH_PAGE ? 2048 : 512;
	cfg_read_confirm = sys_config & SYS_CONFIG_NAND_READ_CONFIRM ? 1 : 0;

	if (cfg_read_confirm)
		nand_info->control_reg = NAND_CTR_RC;
	if (cfg_page_size == 2048)
		nand_info->control_reg |= (NAND_CTR_C2 | NAND_CTR_SZ_8G);
	/*
	  * Always use P3 and I4 to support all NAND,
	  * but we will adjust them after read ID from NAND.
	  */
	nand_info->control_reg |= (NAND_CTR_P3 | NAND_CTR_I4 | NAND_CTR_IE);

	chip->chip_delay = 0;
	chip->controller = &nand_info->controller;
	chip->read_byte = amb_nand_read_byte;
	chip->read_word = amb_nand_read_word;
	chip->write_buf = amb_nand_write_buf;
	chip->read_buf = amb_nand_read_buf;
	chip->verify_buf = amb_nand_verify_buf;
	chip->select_chip = amb_nand_select_chip;
	chip->cmd_ctrl = amb_nand_cmd_ctrl;
	chip->dev_ready = amb_nand_dev_ready;
	chip->waitfunc = amb_nand_waitfunc;
	chip->cmdfunc = amb_nand_cmdfunc;

	chip->options = NAND_NO_AUTOINCR;
	if (nand_info->plat_nand->flash_bbt) {
#ifndef CONFIG_MTD_NAND_AMBARELLA_SOFT_ECC
		chip->options |= NAND_USE_FLASH_BBT;
		chip->bbt_td = &bbt_main_descr;
		chip->bbt_md = &bbt_mirror_descr;
#endif
	}

	nand_info->mtd.priv = chip;
	nand_info->mtd.owner = THIS_MODULE;

	return 0;
}

static int __devinit ambarella_nand_init_chipecc(
	struct ambarella_nand_info *nand_info)
{
	struct nand_chip *chip = &nand_info->chip;
	struct mtd_info	*mtd = &nand_info->mtd;

	if (mtd->writesize == 2048) {
		chip->ecc.layout = &amb_oobinfo_2048;
		chip->badblock_pattern = &amb_2048_bbt_descr;
	} else if (mtd->writesize == 512) {
		chip->ecc.layout = &amb_oobinfo_512;
		chip->badblock_pattern = &amb_512_bbt_descr;
	} else {
		dev_err(nand_info->dev,
			"Unexpected NAND flash writesize %d. Aborting\n",
			mtd->writesize);
		return -1;
	}

	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.size = 512;
	chip->ecc.bytes = 6;
	chip->ecc.hwctl = amb_nand_hwctl;
	chip->ecc.calculate = amb_nand_caculate_ecc;
	chip->ecc.correct = amb_nand_correct_data;
	chip->ecc.write_oob = amb_nand_write_oob_std;

	return 0;
}

static int __devinit ambarella_nand_probe(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_nand_info		*nand_info;
	struct mtd_info				*mtd;
	struct ambarella_platform_nand		*plat_nand;
	struct resource				*wp_res;
	struct resource				*reg_res;
	struct resource				*dma_res;
#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition			*amboot_partitions;
	int					amboot_nr_partitions = 0;
	flpart_meta_t				*meta_table;
	int					meta_numpages, meta_offpage;
	int					from, retlen, found, i;
	int					cmd_nr_partitions = 0;
#ifdef CONFIG_MTD_CMDLINE_PARTS
	struct mtd_partition			*cmd_partitions = NULL;
#endif
#endif

	plat_nand = (struct ambarella_platform_nand *)pdev->dev.platform_data;
	if ((plat_nand == NULL) || (plat_nand->timing == NULL) ||
		(plat_nand->sets == NULL) || (plat_nand->parse_error == NULL) ||
		(plat_nand->request == NULL) || (plat_nand->release == NULL)) {
		dev_err(&pdev->dev, "Can't get platform_data!\n");
		errorCode = - EPERM;
		goto ambarella_nand_probe_exit;
	}

	nand_info = kzalloc(sizeof(struct ambarella_nand_info), GFP_KERNEL);
	if (nand_info == NULL) {
		dev_err(&pdev->dev, "kzalloc for nand nand_info failed!\n");
		errorCode = - ENOMEM;
		goto ambarella_nand_probe_exit;
	}

	reg_res = platform_get_resource_byname(pdev,
		IORESOURCE_MEM, "registers");
	if (reg_res == NULL) {
		dev_err(&pdev->dev, "Get reg_res failed!\n");
		errorCode = -ENXIO;
		goto ambarella_nand_probe_free_info;
	}
	nand_info->regbase = (unsigned char __iomem *)reg_res->start;

	dma_res = platform_get_resource_byname(pdev,
		IORESOURCE_MEM, "dma");
	if (dma_res == NULL) {
		dev_err(&pdev->dev, "Get dma_res failed!\n");
		errorCode = -ENXIO;
		goto ambarella_nand_probe_free_info;
	}
	nand_info->dmabase = ambarella_virt_to_phys(dma_res->start);

	wp_res = platform_get_resource_byname(pdev,
		IORESOURCE_IO, "wp_gpio");
	if (wp_res == NULL) {
		nand_info->wp_gpio = -1;
	} else {
		nand_info->wp_gpio = wp_res->start;

		errorCode = gpio_request(nand_info->wp_gpio, pdev->name);
		if (errorCode < 0) {
			dev_err(&pdev->dev, "Could not get WP GPIO %d\n",
				nand_info->wp_gpio);
			goto ambarella_nand_probe_free_info;
		}

		errorCode = gpio_direction_output(nand_info->wp_gpio,
			GPIO_HIGH);
		if (errorCode < 0) {
			dev_err(&pdev->dev, "Could not Set WP GPIO %d\n",
				nand_info->wp_gpio);
			goto ambarella_nand_probe_free_wp_gpio;
		}
	}

	nand_info->cmd_irq = platform_get_irq_byname(pdev, "ambarella-fio-cmd");
	if (nand_info->cmd_irq <= 0) {
		dev_err(&pdev->dev, "Get cmd_irq failed!\n");
		errorCode = -ENXIO;
		goto ambarella_nand_probe_free_wp_gpio;
	}

	nand_info->dma_irq = platform_get_irq_byname(pdev, "ambarella-fio-dma");
	if (nand_info->dma_irq <= 0) {
		dev_err(&pdev->dev, "Get dma_irq failed!\n");
		errorCode = -ENXIO;
		goto ambarella_nand_probe_free_wp_gpio;
	}

	nand_info->plat_nand = plat_nand;
	nand_info->dev = &pdev->dev;
	spin_lock_init(&nand_info->controller.lock);
	init_waitqueue_head(&nand_info->controller.wq);
	init_waitqueue_head(&nand_info->wq);
	sema_init(&nand_info->system_event_sem, 1);
	atomic_set(&nand_info->irq_flag, 0x7);

	nand_info->dmabuf = dma_alloc_coherent(nand_info->dev,
		AMBARELLA_NAND_DMA_BUFFER_SIZE,
		&nand_info->dmaaddr, GFP_KERNEL);
	if (nand_info->dmabuf == NULL) {
		dev_err(&pdev->dev, "dma_alloc_coherent failed!\n");
		errorCode = -ENOMEM;
		goto ambarella_nand_probe_free_wp_gpio;
	}

	errorCode = request_irq(nand_info->cmd_irq, nand_fiocmd_isr_handler,
			IRQF_SHARED | IRQF_TRIGGER_HIGH,
			dev_name(&pdev->dev), nand_info);
	if (errorCode) {
		dev_err(&pdev->dev, "Could not register IRQ %d!\n",
			nand_info->cmd_irq);
		goto ambarella_nand_probe_free_dma;
	}

	errorCode = request_irq(nand_info->dma_irq, nand_fiodma_isr_handler,
			IRQF_SHARED | IRQF_TRIGGER_HIGH,
			dev_name(&pdev->dev), nand_info);
	if (errorCode) {
		dev_err(&pdev->dev, "Could not register IRQ %d!\n",
			nand_info->dma_irq);
		goto ambarella_nand_probe_free_cmd_irq;
	}

	errorCode = ambarella_dma_request_irq(FIO_DMA_CHAN,
		nand_dma_isr_handler, nand_info);
	if (errorCode) {
		dev_err(&pdev->dev, "Could not request DMA channel %d\n",
			FIO_DMA_CHAN);
		goto ambarella_nand_probe_free_dma_irq;
	}

	errorCode = ambarella_dma_enable_irq(FIO_DMA_CHAN,
		nand_dma_isr_handler);
	if (errorCode) {
		dev_err(&pdev->dev, "Could not enable DMA channel %d\n",
			FIO_DMA_CHAN);
		goto ambarella_nand_probe_free_dma_chan;
	}

#if defined(CONFIG_AMBARELLA_IPC)
	/*
	 * Disablle the IRQs, which by default are enabled when requested.
	 * Otherwise, it causes the unbalenced IRQ enable/disable checking
	 * failed when the first time nand_amb_request() is called.
	 */
#if (CHIP_REV == I1)
	disable_irq(nand_info->cmd_irq);
	disable_irq(nand_info->dma_irq);
#endif
#endif

	ambarella_nand_init_chip(nand_info);

	mtd = &nand_info->mtd;
	errorCode = nand_scan_ident(mtd, plat_nand->sets->nr_chips, NULL);
	if (errorCode)
		goto ambarella_nand_probe_mtd_error;

	errorCode = ambarella_nand_init_chipecc(nand_info);
	if (errorCode)
		goto ambarella_nand_probe_mtd_error;

	errorCode = ambarella_nand_config_flash(nand_info);
	if (errorCode)
		goto ambarella_nand_probe_mtd_error;

	errorCode = nand_scan_tail(mtd);
	if (errorCode)
		goto ambarella_nand_probe_mtd_error;

	amboot_nr_partitions = 0;
	amboot_partitions =
		kzalloc((PART_MAX+CMDLINE_PART_MAX)*sizeof(struct mtd_partition),
		GFP_KERNEL);

	meta_table = kzalloc(sizeof(flpart_meta_t), GFP_KERNEL);

#ifdef CONFIG_MTD_PARTITIONS
#ifdef CONFIG_AMBPTB_PARTITION
	/* struct  flpart_table_t contains the meta data which contains the
	  * partition info.
	  * meta_offpage indicate the offset from each block
	  * meta_numpages indicate the size of the meta data
	  */
	meta_offpage = sizeof(flpart_table_t) / mtd->writesize;
	meta_numpages = sizeof(flpart_meta_t) / mtd->writesize;
	if (sizeof(flpart_meta_t) % mtd->writesize)
		meta_numpages++;

	if (meta_numpages * mtd->writesize > AMBARELLA_NAND_DMA_BUFFER_SIZE) {
		dev_err(nand_info->dev,
			"%s: The size of meta data is too large\n", __func__);
		errorCode = -ENOMEM;
		goto ambarella_nand_probe_mtd_error;
	}

	/* find the meta data, start from the second block */
	found = 0;
	for (from = mtd->erasesize; from < mtd->size; from += mtd->erasesize) {
		if (mtd->block_isbad(mtd, from)) {
			continue;
		}

		errorCode = mtd->read(mtd,
			from + meta_offpage * mtd->writesize,
			meta_numpages * mtd->writesize,
			&retlen, (u8 *)meta_table);
		if (errorCode < 0) {
			dev_info(nand_info->dev,
				"%s: Can't meta data!\n", __func__);
			goto ambarella_nand_probe_mtd_error;
		}

		if (meta_table->magic == PTB_META_MAGIC) {
			found = 1;
			break;
		} else if (meta_table->magic == PTB_META_MAGIC2) {
			found = 2;
			break;
		}
	}

	if (found == 0) {
		dev_err(nand_info->dev, "%s: meta appears damaged...\n", __func__);
		errorCode = -EINVAL;
		goto ambarella_nand_probe_mtd_error;
	}

	dev_info(nand_info->dev, "%s: Partition infomation found!\n", __func__);

	/* if this partition isn't located in NAND, fake its nblk to 0, this
	 * feature start from the second version of flpart_meta_t. */
	if (found > 1) {
		for (i = 0; i < PART_MAX; i++) {
			if (meta_table->part_dev[i] != BOOT_DEV_NAND)
				meta_table->part_info[i].nblk = 0;
		}
	}

	found = 0;
	for (i = 0; i < PART_MAX; i++) {
#ifdef CONFIG_MTD_NAND_AMBARELLA_ENABLE_FULL_PTB
	/*
	  * found swp partition, and with it as a benchmark to adjust each
	  * partitions's offset
	  * adjust rules:
	  * 1. if before swp partition,
	  *	if the partition's nblk (size) is 0, make its sblk and nblk
	  *	equal to the previous one;
	  * 2. if after swp partition,
	  *	if the partition's nblk (size) is 0, skip the partition;
	  */
		if (!found && !strncmp(meta_table->part_info[i].name, "swp", 3)) {
			found = 1;
		}
		if (found) {
			if (meta_table->part_info[i].nblk == 0)
				continue;
		} else {
			/* bst partition should be always exist */
			if (i > 0 && meta_table->part_info[i].nblk == 0) {
				meta_table->part_info[i].sblk =
					meta_table->part_info[i-1].sblk;
				meta_table->part_info[i].nblk =
					meta_table->part_info[i-1].nblk;
			}
		}
#else
		if (meta_table->part_info[i].nblk == 0)
			continue;
#endif
		strcpy(part_name[i], meta_table->part_info[i].name);
		amboot_partitions[amboot_nr_partitions].name = part_name[i];
		amboot_partitions[amboot_nr_partitions].offset = meta_table->part_info[i].sblk * mtd->erasesize;
		amboot_partitions[amboot_nr_partitions].size = meta_table->part_info[i].nblk * mtd->erasesize;
		amboot_nr_partitions++;
	}
#endif

#ifdef CONFIG_MTD_CMDLINE_PARTS
	nand_info->mtd.name = "ambnand";
	/* get partitions definition from cmdline */
	cmd_nr_partitions = parse_mtd_partitions(&nand_info->mtd,
				part_probes, &cmd_partitions, 0);
	if (cmd_nr_partitions <= 0)
		goto ambarella_nand_probe_add_partitions;

	if (cmd_nr_partitions > CMDLINE_PART_MAX) {
		dev_info(nand_info->dev, "Too many partitionings, truncating\n");
		cmd_nr_partitions = CMDLINE_PART_MAX;
	}

	/* if cmdline don't define the partition offset, we should modify the
	  * offset to make it append to the existent partitions
	  */
	if (cmd_partitions->offset == 0) {
		struct mtd_partition *p_cmdpart = cmd_partitions;
		struct mtd_partition *p_ambpart = &amboot_partitions[0];
		/* find the last partition getting from amboot */
		for (i = 1; i < amboot_nr_partitions; i++) {
			if (amboot_partitions[i].offset > p_ambpart->offset) {
				p_ambpart = &amboot_partitions[i];
			}
		}

		for (i = 0; i < cmd_nr_partitions; i++) {
			if (i > 0)
				p_ambpart = cmd_partitions + i - 1;

			p_cmdpart->offset = p_ambpart->offset + p_ambpart->size;

			if (p_cmdpart->offset + p_cmdpart->size >
					nand_info->mtd.size) {
				p_cmdpart->size =
					nand_info->mtd.size - p_cmdpart->offset;
				dev_info(nand_info->dev,
					"partitioning exceeds flash size, truncating\n");
				cmd_nr_partitions = i + 1;
				break;
			}

			p_cmdpart++;
		}
	}

	/* append the cmdline partitions to partitions from amboot. */
	for (i = 0; i < cmd_nr_partitions; i++) {
		memcpy(&amboot_partitions[amboot_nr_partitions + i],
			cmd_partitions++, sizeof(struct mtd_partition));
	}

ambarella_nand_probe_add_partitions:
#endif
	i = 0;
	if (cmd_nr_partitions >= 0)
		i = cmd_nr_partitions;
	add_mtd_partitions(mtd, amboot_partitions, amboot_nr_partitions + i);

	kfree(meta_table);
	kfree(amboot_partitions);
#else
	add_mtd_device(&nand_info->mtd);
#endif

	platform_set_drvdata(pdev, nand_info);

	nand_info->origin_clk = get_ahb_bus_freq_hz() / 1000; /* in Khz */
	nand_info->origin_timing = plat_nand->timing;
	memcpy(&nand_info->current_timing, plat_nand->timing,
			sizeof(struct ambarella_nand_timing));

	nand_info->system_event.notifier_call =	ambarella_nand_system_event;
	ambarella_register_event_notifier(&nand_info->system_event);

	goto ambarella_nand_probe_exit;

ambarella_nand_probe_mtd_error:
	ambarella_dma_disable_irq(FIO_DMA_CHAN, nand_dma_isr_handler);

ambarella_nand_probe_free_dma_chan:
	ambarella_dma_free_irq(FIO_DMA_CHAN, nand_dma_isr_handler);

ambarella_nand_probe_free_dma_irq:
	free_irq(nand_info->dma_irq, nand_info);

ambarella_nand_probe_free_cmd_irq:
	free_irq(nand_info->cmd_irq, nand_info);

ambarella_nand_probe_free_dma:
	dma_free_coherent(nand_info->dev,
		AMBARELLA_NAND_DMA_BUFFER_SIZE,
		nand_info->dmabuf, nand_info->dmaaddr);

ambarella_nand_probe_free_wp_gpio:
	if (nand_info->wp_gpio >= 0)
		gpio_free(nand_info->wp_gpio);

ambarella_nand_probe_free_info:
	kfree(nand_info);

ambarella_nand_probe_exit:

	return errorCode;
}

static int __devexit ambarella_nand_remove(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_nand_info		*nand_info;

	nand_info = (struct ambarella_nand_info *)platform_get_drvdata(pdev);

	if (nand_info) {
		ambarella_unregister_event_notifier(&nand_info->system_event);

		platform_set_drvdata(pdev, NULL);

		errorCode = ambarella_dma_disable_irq(FIO_DMA_CHAN,
			nand_dma_isr_handler);
		ambarella_dma_free_irq(FIO_DMA_CHAN, nand_dma_isr_handler);
		free_irq(nand_info->dma_irq, nand_info);
		free_irq(nand_info->cmd_irq, nand_info);

		dma_free_coherent(nand_info->dev,
			AMBARELLA_NAND_DMA_BUFFER_SIZE,
			nand_info->dmabuf, nand_info->dmaaddr);

		nand_release(&nand_info->mtd);

		if (nand_info->wp_gpio >= 0) {
			gpio_direction_output(nand_info->wp_gpio, GPIO_LOW);
			gpio_free(nand_info->wp_gpio);
		}
		kfree(nand_info);
	}

	return errorCode;
}

#ifdef CONFIG_PM
static int ambarella_nand_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int					errorCode = 0;
	struct ambarella_nand_info		*nand_info;

	nand_info = platform_get_drvdata(pdev);
	nand_info->suspend = 1;
	disable_irq(nand_info->dma_irq);
	disable_irq(nand_info->cmd_irq);

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);

	return errorCode;
}

static int ambarella_nand_resume(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct ambarella_nand_info		*nand_info;

	nand_info = platform_get_drvdata(pdev);
#if !defined(CONFIG_AMBARELLA_IPC)
	amb_nand_set_timing(nand_info, &nand_info->current_timing);
#endif
	nand_info->suspend = 0;
	enable_irq(nand_info->dma_irq);
	enable_irq(nand_info->cmd_irq);

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

	return errorCode;
}
#endif

static struct platform_driver amb_nand_driver = {
	.probe		= ambarella_nand_probe,
	.remove		= __devexit_p(ambarella_nand_remove),
#ifdef CONFIG_PM
	.suspend	= ambarella_nand_suspend,
	.resume		= ambarella_nand_resume,
#endif
	.driver = {
		.name	= "ambarella-nand",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_nand_init(void)
{
	return platform_driver_register(&amb_nand_driver);
}

static void __exit ambarella_nand_exit(void)
{
	platform_driver_unregister(&amb_nand_driver);
}

module_init(ambarella_nand_init);
module_exit(ambarella_nand_exit);

MODULE_AUTHOR("Cao Rongrong & Chien-Yang Chen");
MODULE_DESCRIPTION("Ambarella Media processor NAND Controller Driver");
MODULE_LICENSE("GPL");

