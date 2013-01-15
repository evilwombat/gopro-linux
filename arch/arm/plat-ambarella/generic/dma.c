/*
 * arch/arm/plat-ambarella/generic/dma.c
 *
 * History:
 *	2008/03/05 - [Chien-Yang Chen] created file
 *	2009/01/12 - [Anthony Ginger] Port to 2.6.28
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>

#include <asm/system.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/dma.h>

struct dma_s G_dma;
static spinlock_t dma_lock;

#ifdef CONFIG_AMBARELLA_DMA_PROC
static const char dma_proc_name[] = "dma";
static struct proc_dir_entry *dma_file;

static int ambarella_dma_proc_read(char *buf, char **start, off_t off,
	int len, int *eof, void *data)
{
	char *p = buf;

	p += sprintf(p, "You can add something here to debug DMA \n");

	return p - buf;
}
#endif

static irqreturn_t ambarella_dma_int_handler(int irq, void *dev_id)
{
	struct dma_s				*dma;
	int					i;
	int					j;
	u32					ireg;

	dma = (struct dma_s *)dev_id;

	ireg = amba_readl(DMA_INT_REG);
	for (i = 0; i < NUM_DMA_CHANNELS; i++) {
		if (ireg & (0x1 << i)) {
			dma->chan[i].status = amba_readl(DMA_CHAN_STA_REG(i));
			amba_writel(DMA_CHAN_STA_REG(i), 0);

			for (j = 0; j < dma->chan[i].irq_count; j++) {
				if (dma->chan[i].irq[j].enabled == 1) {
					dma->chan[i].irq[j].handler(
						dma->chan[i].irq[j].harg,
						dma->chan[i].status);
				}
			}
		}
	}

	return IRQ_HANDLED;
}

#if (DMA_SUPPORT_DMA_FIOS == 1)
static irqreturn_t ambarella_dma_fios_int_handler(int irq, void *dev_id)
{
	struct dma_s				*dma;
	int					i;
	u32					ireg;

	dma = (struct dma_s *)dev_id;

	ireg = amba_readl(DMA_FIOS_INT_REG);
	if (ireg & DMA_INT_CHAN0) {
		dma->chan[FIO_DMA_CHAN].status =
			amba_readl(DMA_FIOS_CHAN_STA_REG(FIO_DMA_CHAN));
		amba_writel(DMA_FIOS_CHAN_STA_REG(FIO_DMA_CHAN), 0);

		for (i = 0; i < dma->chan[FIO_DMA_CHAN].irq_count; i++) {
			if (dma->chan[FIO_DMA_CHAN].irq[i].enabled == 1) {
				dma->chan[FIO_DMA_CHAN].irq[i].handler(
					dma->chan[FIO_DMA_CHAN].irq[i].harg,
					dma->chan[FIO_DMA_CHAN].status);
			}
		}
	}

	return IRQ_HANDLED;
}
#endif

int ambarella_dma_request_irq(int chan,
	ambarella_dma_handler handler, void *harg)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto ambarella_dma_request_irq_exit_na;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		goto ambarella_dma_request_irq_exit_na;
	}

	spin_lock_irqsave(&dma_lock, flags);
	if (unlikely(G_dma.chan[chan].irq_count >
		MAX_DMA_CHANNEL_IRQ_HANDLERS)) {
		pr_err("%s: chan[%d]'s irq_count[%d] > "
			"MAX_DMA_CHANNEL_IRQ_HANDLERS[%d]!\n",
			__func__, chan, G_dma.chan[chan].irq_count,
			MAX_DMA_CHANNEL_IRQ_HANDLERS);
		retval = -EINVAL;
		goto ambarella_dma_request_irq_exit;
	}

	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == NULL) {
			G_dma.chan[chan].irq[i].enabled = 0;
			G_dma.chan[chan].irq[i].handler = handler;
			G_dma.chan[chan].irq[i].harg = harg;
			G_dma.chan[chan].irq_count++;
			break;
		}
	}

ambarella_dma_request_irq_exit:
	spin_unlock_irqrestore(&dma_lock, flags);

ambarella_dma_request_irq_exit_na:
	return retval;
}
EXPORT_SYMBOL(ambarella_dma_request_irq);

void ambarella_dma_free_irq(int chan, ambarella_dma_handler handler)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		return;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		return;
	}

	spin_lock_irqsave(&dma_lock, flags);
	if (unlikely(G_dma.chan[chan].irq_count >
		MAX_DMA_CHANNEL_IRQ_HANDLERS)) {
		pr_err("%s: chan[%d]'s irq_count[%d] > "
			"MAX_DMA_CHANNEL_IRQ_HANDLERS[%d]!\n",
			__func__, chan, G_dma.chan[chan].irq_count,
			MAX_DMA_CHANNEL_IRQ_HANDLERS);
		retval = -EINVAL;
		goto ambarella_dma_free_irq_exit;
	}

	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == handler) {
			G_dma.chan[chan].irq[i].enabled = 0;
			G_dma.chan[chan].irq[i].handler = NULL;
			G_dma.chan[chan].irq[i].harg = NULL;
			G_dma.chan[chan].irq_count--;
			break;
		}
	}

	for (i = i + 1; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler != NULL) {
			G_dma.chan[chan].irq[i - 1].enabled =
				G_dma.chan[chan].irq[i].enabled;
			G_dma.chan[chan].irq[i - 1].handler =
				G_dma.chan[chan].irq[i].handler;
			G_dma.chan[chan].irq[i - 1].harg =
				G_dma.chan[chan].irq[i].harg;
			G_dma.chan[chan].irq[i].handler = NULL;
			G_dma.chan[chan].irq[i].harg = NULL;
			G_dma.chan[chan].irq[i].enabled = 0;
		}
	}

ambarella_dma_free_irq_exit:
	spin_unlock_irqrestore(&dma_lock, flags);
}
EXPORT_SYMBOL(ambarella_dma_free_irq);

int ambarella_dma_enable_irq(int chan, ambarella_dma_handler handler)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto ambarella_dma_enable_irq_na;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		goto ambarella_dma_enable_irq_na;
	}

	spin_lock_irqsave(&dma_lock, flags);
	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == NULL) {
			retval = -EINVAL;
			pr_err("%s: can't find 0x%x!\n",
				__func__, (u32)handler);
			break;
		}

		if (G_dma.chan[chan].irq[i].handler == handler) {
			G_dma.chan[chan].irq[i].enabled = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&dma_lock, flags);

ambarella_dma_enable_irq_na:
	return retval;
}
EXPORT_SYMBOL(ambarella_dma_enable_irq);

int ambarella_dma_disable_irq(int chan, ambarella_dma_handler handler)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto ambarella_dma_disable_irq_na;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		goto ambarella_dma_disable_irq_na;
	}

	spin_lock_irqsave(&dma_lock, flags);
	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == NULL) {
			retval = -EINVAL;
			pr_err("%s: can't find 0x%x!\n",
				__func__, (u32)handler);
			break;
		}

		if (G_dma.chan[chan].irq[i].handler == handler) {
			G_dma.chan[chan].irq[i].enabled = 0;
			break;
		}
	}
	spin_unlock_irqrestore(&dma_lock, flags);

ambarella_dma_disable_irq_na:
	return retval;
}
EXPORT_SYMBOL(ambarella_dma_disable_irq);

#if (DMA_SUPPORT_DMA_FIOS == 1)
static inline int amb_req_dma_fios(ambarella_dma_req_t * req, int chan)
{
	int					retval = 0;
	u32					ctr = 0;

	if (unlikely(req->xfr_count > 0x003fffff)) {
		pr_err("%s: xfr_count[0x%x] out of range!\n",
			__func__, req->xfr_count);
		retval = -EINVAL;
		goto amb_req_dma_fios_exit;
	}

	amba_writel(DMA_FIOS_CHAN_STA_REG(chan), 0);
	if (req->next == NULL) {
		amba_writel(DMA_FIOS_CHAN_SRC_REG(chan), req->src);
		amba_writel(DMA_FIOS_CHAN_DST_REG(chan), req->dst);

		ctr |= req->attr | req->xfr_count;
		ctr &= ~DMA_CHANX_CTR_D;
		ctr |= DMA_CHANX_CTR_EN;
	} else {/* Descriptor mode */
		amba_writel(DMA_FIOS_CHAN_DA_REG(chan), (u32)req);

		ctr |= DMA_CHANX_CTR_D;
		ctr |= DMA_CHANX_CTR_EN;
	}
	amba_writel(DMA_FIOS_CHAN_CTR_REG(chan), ctr);

amb_req_dma_fios_exit:
	return retval;
}
#endif

static inline int amb_req_dma(ambarella_dma_req_t * req, int chan)
{
	int					retval = 0;
	u32					ctr = 0;

	if (unlikely(req->xfr_count > 0x003fffff)) {
		pr_err("%s: xfr_count[0x%x] out of range!\n",
			__func__, req->xfr_count);
		retval = -EINVAL;
		goto amb_req_dma_exit;
	}

	amba_writel(DMA_CHAN_STA_REG(chan), 0);
	if (req->next == NULL) {
		amba_writel(DMA_CHAN_SRC_REG(chan), req->src);
		amba_writel(DMA_CHAN_DST_REG(chan), req->dst);

		ctr |= (req->attr | req->xfr_count);
		ctr &= ~DMA_CHANX_CTR_D;
		ctr |= DMA_CHANX_CTR_EN;

	} else {/* Descriptor mode */
		amba_writel((u32) req, DMA_CHAN_DA_REG(chan));

		ctr |= DMA_CHANX_CTR_D;
		ctr |= DMA_CHANX_CTR_EN;
	}
	amba_writel(DMA_CHAN_CTR_REG(chan), ctr);

amb_req_dma_exit:
	return retval;
}

int ambarella_dma_xfr(ambarella_dma_req_t *req, int chan)
{
	int					retval = 0;
#ifdef CHECK_DMA_CHAN_USE_FLAG
	unsigned long				flags;
#endif

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto ambarella_dma_xfr_exit;
	}

	if (unlikely(req == NULL)) {
		pr_err("%s: req is NULL!\n", __func__);
		retval = -EINVAL;
		goto ambarella_dma_xfr_exit;
	}

#ifdef CHECK_DMA_CHAN_USE_FLAG
	spin_lock_irqsave(&dma_lock, flags);
	G_dma.chan[chan].use_flag = 1;
	spin_unlock_irqrestore(&dma_lock, flags);
#endif

#if (DMA_SUPPORT_DMA_FIOS == 1)
	if(chan == 0) {
		retval = amb_req_dma_fios(req, chan);
	} else {
		retval = amb_req_dma(req, chan);
	}
#else
	retval = amb_req_dma(req, chan);
#endif

ambarella_dma_xfr_exit:
	return retval;
}
EXPORT_SYMBOL(ambarella_dma_xfr);

int ambarella_dma_desc_xfr(dma_addr_t desc_addr, int chan)
{
	int					retval = 0;

	if (unlikely(desc_addr == 0)) {
		pr_err("%s: desc_addr is NULL!\n", __func__);
		retval = -EINVAL;
		goto ambarella_dma_desc_xfr_exit;
	}

	if (unlikely((desc_addr & 0x7) != 0)) {
		pr_err("%s: desc_addr isn't aligned!\n", __func__);
		retval = -EINVAL;
		goto ambarella_dma_desc_xfr_exit;
	}

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto ambarella_dma_desc_xfr_exit;
	}

	amba_writel(DMA_CHAN_DA_REG(chan), desc_addr);
	amba_writel(DMA_CHAN_CTR_REG(chan), DMA_CHANX_CTR_EN | DMA_CHANX_CTR_D);

ambarella_dma_desc_xfr_exit:
	return retval;
}
EXPORT_SYMBOL(ambarella_dma_desc_xfr);

#if defined(CONFIG_MTD_NAND_AMBARELLA) && defined(CONFIG_AMBARELLA_IPC) && defined(CONFIG_SUPPORT_ANDROID_TT)
int __init ambarella_init_dma(void)
{
	int	retval = 0;
	int	i;
	struct dma_s *dma = &G_dma;

	spin_lock_init(&dma_lock);
	memset(&G_dma, 0x0, sizeof(G_dma));

	retval = request_irq(DMA_IRQ, ambarella_dma_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "ambarella-dma", dma);
	if (retval) {
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_IRQ, retval);
		goto ambarella_init_dma_exit;
	}

	for (i = 1; i < NUM_DMA_CHANNELS; i++) {
		amba_writel(DMA_CHAN_STA_REG(i), 0);
		amba_writel(DMA_CHAN_CTR_REG(i), 0x38000000);
	}

	retval = request_irq(DMA_FIOS_IRQ, ambarella_dma_fios_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "ambarella-fios-dma", dma);
	if (retval){
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_FIOS_IRQ, retval);
		goto ambarella_init_dma_exit;
	} else {
#if (CHIP_REV == I1)
		/* Disablle DMA IRQ by default.*/
		/* Enable it in nand_amb_request(). */
		disable_irq(DMA_FIOS_IRQ);
#endif
	}
ambarella_init_dma_exit:
	return retval;
}
#elif defined(CONFIG_MTD_NAND_AMBARELLA) && defined(CONFIG_AMBARELLA_IPC)
int __init ambarella_init_dma(void)
{
	int					retval = 0;
	struct dma_s				*dma = &G_dma;

	spin_lock_init(&dma_lock);
	memset(&G_dma, 0x0, sizeof(G_dma));

	retval = request_irq(DMA_FIOS_IRQ, ambarella_dma_fios_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "ambarella-fios-dma", dma);
	if (retval){
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_FIOS_IRQ, retval);
	} else {
#if (CHIP_REV == I1)
		/* Disablle DMA IRQ by default.	    */
		/* Enable it in nand_amb_request(). */
		disable_irq(DMA_FIOS_IRQ);
#endif
	}

	return retval;
}
#else	/* NAND_AMBARELLA && AMBARELLA_IPC */

int __init ambarella_init_dma(void)
{
	int					retval = 0;
	int					i;
	struct dma_s				*dma = &G_dma;

	spin_lock_init(&dma_lock);
	memset(&G_dma, 0x0, sizeof(G_dma));

	retval = request_irq(DMA_IRQ, ambarella_dma_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "ambarella-dma", dma);
	if (retval) {
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_IRQ, retval);
		goto ambarella_init_dma_exit;
	}

	for (i = 0; i < NUM_DMA_CHANNELS; i++) {
		amba_writel(DMA_CHAN_STA_REG(i), 0);
		amba_writel(DMA_CHAN_CTR_REG(i), 0x38000000);
	}

#if (DMA_SUPPORT_DMA_FIOS == 1)
	retval = request_irq(DMA_FIOS_IRQ, ambarella_dma_fios_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "ambarella-fios-dma", dma);
	if (retval){
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_FIOS_IRQ, retval);
		goto ambarella_init_dma_exit;
	}
	amba_writel(DMA_FIOS_CHAN_STA_REG(0), 0);
	amba_writel(DMA_FIOS_CHAN_CTR_REG(0), 0x38000000);
#endif

#ifdef CONFIG_AMBARELLA_DMA_PROC
	dma_file = create_proc_entry(dma_proc_name, S_IRUGO | S_IWUSR,
		get_ambarella_proc_dir());
	if (dma_file == NULL) {
		retval = -ENOMEM;
		pr_err("%s: for %s fail!\n", __func__, dma_proc_name);
	} else {
		dma_file->read_proc = ambarella_dma_proc_read;
		dma_file->write_proc = NULL;
	}
#endif

ambarella_init_dma_exit:
	return retval;
}
#endif	/* NAND_AMBARELLA && AMBARELLA_IPC */

