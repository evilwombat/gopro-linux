/*
 * sound/soc/ambarella_pcm.c
 *
 * History:
 *	2008/03/03 - [Eric Lee] created file
 *	2008/04/16 - [Eric Lee] Removed the compiling warning
 *	2008/08/07 - [Cao Rongrong] Fix the buffer bug,eg: size and allocation
 *	2008/11/14 - [Cao Rongrong] Support pause and resume
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *	2009/03/05 - [Cao Rongrong] Update from 2.6.22.10
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
 *	2009/06/30 - [Cao Rongrong] Fix last_desc bug
 *	2010/10/25 - [Cao Rongrong] Port to 2.6.36+
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38
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
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <mach/hardware.h>
#include <mach/dma.h>

#include "ambarella_pcm.h"

#define AMBA_MAX_DESC_NUM		128
#define AMBA_MIN_DESC_NUM		2
#define AMBA_PERIOD_BYTES_MAX		(128 * 1024)
#define AMBA_PERIOD_BYTES_MIN		32
#define AMBA_BUFFER_BYTES_MAX		(256 * 1024)


struct ambarella_runtime_data {
	int working;
	struct ambarella_pcm_dma_params *dma_data;

	ambarella_dma_req_t *dma_desc_array; // FIXME this
	dma_addr_t dma_desc_array_phys;
	int channel;		/* Physical DMA channel */
	int ndescr;		/* Number of descriptors */
	int last_descr;		/* Record lastest DMA done descriptor number */

	u32 *dma_rpt_buf;
	dma_addr_t dma_rpt_phys;

	spinlock_t lock;
	wait_queue_head_t wq;
};

static const struct snd_pcm_hardware ambarella_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_BLOCK_TRANSFER |
				SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_PAUSE |
				SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= SNDRV_PCM_RATE_8000_48000,
	.rate_min		= 8000,
	.rate_max		= 48000,
	.period_bytes_min	= AMBA_PERIOD_BYTES_MIN,
	.period_bytes_max	= AMBA_PERIOD_BYTES_MAX,
	.periods_min		= AMBA_MIN_DESC_NUM,
	.periods_max		= AMBA_MAX_DESC_NUM,
	.buffer_bytes_max	= AMBA_BUFFER_BYTES_MAX,
};



/*
 * DMA transfer request
 * param me Point to object self
 * param req Point to DMA request array
 * param chan DMA channel
 * param ndes Number of descriptors in descriptor chain
 * return 0 = success; otherwise, failure
 */
static int ambarella_dai_dma_xfr(struct ambarella_runtime_data *prtd)
{
	int retval, i;

	for(i = 0; i < prtd->ndescr; i++)
		prtd->dma_desc_array[i].attr &= ~DMA_DESC_EOC;

	retval = ambarella_dma_desc_xfr(
		prtd->dma_desc_array_phys + sizeof(ambarella_dma_req_t) * prtd->last_descr,
		prtd->channel);

	prtd->working = 1;

	return retval;
}

/*
 * Stop DMA transfer
 * param me Point to the object self
 * return 0 = success; otherwise, failure.
 */
int dai_dma_stop(struct ambarella_runtime_data *prtd)
{
	int descr, i;

	for(i = 2; i <= 4; i++){
		descr = (prtd->last_descr + i) % prtd->ndescr;
		prtd->dma_desc_array[descr].attr |= DMA_DESC_EOC;
	}

	return 0;
}

static void dai_dma_handler(void *dev_id)
{
	struct snd_pcm_substream *substream = dev_id;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

	u32 *rpt;
	int cur_descr;

	cur_descr = prtd->last_descr;

	prtd->last_descr++;
	if (prtd->last_descr >= prtd->ndescr) {
		prtd->last_descr = 0;
	}
	snd_pcm_period_elapsed(substream);

	/* Check if stop dma chain */
	rpt = &prtd->dma_rpt_buf[cur_descr];
	if (*rpt & 0x10000000) { /* Descriptor chain done */
		prtd->working = 0;
		*rpt &= (~0x10000000);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/* dai_tx_disable() */
			amba_clrbitsl(I2S_INIT_REG, 0x4);
			/* dai_fifo_rst() */
			if(!amba_tstbitsl(I2S_INIT_REG, 0x6))
				amba_setbitsl(I2S_INIT_REG, 0x1);
		} else {
			/* dai_rx_disable() */
			amba_clrbitsl(I2S_INIT_REG, 0x2);
			/* dai_fifo_rst() */
			if(!amba_tstbitsl(I2S_INIT_REG, 0x6))
				amba_setbitsl(I2S_INIT_REG, 0x1);
		}

		wake_up(&prtd->wq);
	}

	//if (*rpt & 0x08000000)  /* Descriptor DMA operation done */
	//	*rpt &= (~0x08000000);
}

static void dai_rx_dma_handler(void *dev_id, u32 status)
{
	dai_dma_handler(dev_id);
}

static void dai_tx_dma_handler(void *dev_id, u32 status)
{
	dai_dma_handler(dev_id);
}

/* this may get called several times by oss emulation */
static int ambarella_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct ambarella_pcm_dma_params *dma_data;
	size_t totsize = params_buffer_bytes(params);
	size_t period = params_period_bytes(params);
	ambarella_dma_req_t *dma_desc;
	dma_addr_t dma_buff_phys, next_desc_phys, next_rpt;
	int ret, i;

	dma_data = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	if (!dma_data)
		return -ENODEV;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = totsize;

	if (prtd->dma_data)
		return 0;

	prtd->dma_data = dma_data;
	prtd->working = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		prtd->channel = I2S_TX_DMA_CHAN;
		ret = ambarella_dma_request_irq(I2S_TX_DMA_CHAN,
				dai_tx_dma_handler, substream);
		if (ret < 0)
			return ret;
		ret = ambarella_dma_enable_irq(I2S_TX_DMA_CHAN,
				dai_tx_dma_handler);
		if (ret < 0)
			return ret;

	} else {
		prtd->channel = I2S_RX_DMA_CHAN;
		ret = ambarella_dma_request_irq(I2S_RX_DMA_CHAN,
				dai_rx_dma_handler, substream);
		if (ret < 0)
			return ret;
		ret = ambarella_dma_enable_irq(I2S_RX_DMA_CHAN,
				dai_rx_dma_handler);
		if (ret < 0)
			return ret;
	}

	spin_lock_irq(&prtd->lock);

	dma_desc = prtd->dma_desc_array;
	next_desc_phys = prtd->dma_desc_array_phys;
	next_rpt = prtd->dma_rpt_phys;
	dma_buff_phys = runtime->dma_addr;

	prtd->ndescr = 0;
	prtd->last_descr = 0;
	do {
		next_desc_phys += sizeof(ambarella_dma_req_t);
		dma_desc->next = (struct ambarella_dma_req_s *) next_desc_phys;

		if (period > totsize)
			period = totsize;

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dma_desc->attr = DMA_DESC_NI | DMA_DESC_TS_4B |
				DMA_DESC_BLK_32B | DMA_DESC_IE | DMA_DESC_ST |
				DMA_DESC_ID | DMA_DESC_RM;
			dma_desc->src = dma_buff_phys;
			dma_desc->dst = prtd->dma_data->dev_addr;
		} else {
			dma_desc->attr = DMA_DESC_NI | DMA_DESC_TS_2B |
				DMA_DESC_BLK_32B | DMA_DESC_IE | DMA_DESC_ST |
				DMA_DESC_ID | DMA_DESC_WM;
			dma_desc->src = prtd->dma_data->dev_addr;
			dma_desc->dst = dma_buff_phys;
		}
		dma_desc->xfr_count = period;
		dma_desc->rpt = next_rpt;

		dma_buff_phys += period;
		next_rpt += sizeof(dma_addr_t);
		dma_desc++;
		prtd->ndescr++;
	} while (totsize -= period);
	dma_desc[-1].next = (struct ambarella_dma_req_s *) prtd->dma_desc_array_phys;

	for (i = 0; i < prtd->ndescr; i++)
		prtd->dma_rpt_buf[i] = 0;

	spin_unlock_irq(&prtd->lock);

	return 0;
}

static int ambarella_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

	if (prtd->dma_data) {
		/* Wait DMA stop before disable DMA irq */
		wait_event_timeout(prtd->wq, (prtd->working == 0), 3 * HZ);
		/* Disable and free DMA irq */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ambarella_dma_disable_irq(I2S_TX_DMA_CHAN,
					dai_tx_dma_handler);
			ambarella_dma_free_irq(I2S_TX_DMA_CHAN,
					dai_tx_dma_handler);
		} else {
			ambarella_dma_disable_irq(I2S_RX_DMA_CHAN,
					dai_rx_dma_handler);
			ambarella_dma_free_irq(I2S_RX_DMA_CHAN,
					dai_rx_dma_handler);
		}
		prtd->dma_data = NULL;
		prtd->ndescr = 0;
		prtd->last_descr = 0;
	}

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int ambarella_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct ambarella_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	spin_lock_irq(&prtd->lock);
	dai_dma_stop(prtd);
	spin_unlock_irq(&prtd->lock);

	wait_event_interruptible_timeout(prtd->wq, (prtd->working == 0), 3 * HZ);

	return ret;
}

static int ambarella_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;
	int ret = 0;

	spin_lock_irq(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			ambarella_dai_dma_xfr(prtd);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			dai_dma_stop(prtd);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock_irq(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t ambarella_pcm_pointer(
	struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

	return prtd->last_descr * runtime->period_size;
}

static int ambarella_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd;
	int ret = 0;

	snd_soc_set_runtime_hwparams(substream, &ambarella_pcm_hardware);

	/* Add a rule to enforce the DMA buffer align. */
	ret = snd_pcm_hw_constraint_step(runtime, 0,
		SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 32);
	if (ret)
		goto ambarella_pcm_open_exit;

	ret = snd_pcm_hw_constraint_step(runtime, 0,
		SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 32);
	if (ret)
		goto ambarella_pcm_open_exit;

	ret = snd_pcm_hw_constraint_integer(runtime,
		SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto ambarella_pcm_open_exit;

	prtd = kzalloc(sizeof(struct ambarella_runtime_data), GFP_KERNEL);
	if (prtd == NULL) {
		ret = -ENOMEM;
		goto ambarella_pcm_open_exit;
	}

	prtd->dma_desc_array = dma_alloc_coherent(substream->pcm->card->dev,
		AMBA_MAX_DESC_NUM * sizeof(ambarella_dma_req_t),
		&prtd->dma_desc_array_phys, GFP_KERNEL);
	if (!prtd->dma_desc_array) {
		ret = -ENOMEM;
		goto ambarella_pcm_open_free_prtd;
	}

	prtd->dma_rpt_buf = dma_alloc_coherent(substream->pcm->card->dev,
					AMBA_MAX_DESC_NUM * sizeof(prtd->dma_rpt_buf),
					&prtd->dma_rpt_phys, GFP_KERNEL);
	if(prtd->dma_rpt_buf == NULL){
		dev_err(substream->pcm->card->dev,
			"No memory for dma_rpt_buf\n");
		goto ambarella_pcm_open_free_dma_desc_array;
	}

	spin_lock_init(&prtd->lock);
	init_waitqueue_head(&prtd->wq);

	runtime->private_data = prtd;
	goto ambarella_pcm_open_exit;

ambarella_pcm_open_free_dma_desc_array:
	dma_free_coherent(substream->pcm->card->dev,
		AMBA_MAX_DESC_NUM * sizeof(ambarella_dma_req_t),
		prtd->dma_desc_array, prtd->dma_desc_array_phys);

ambarella_pcm_open_free_prtd:
	kfree(prtd);

ambarella_pcm_open_exit:
	return ret;
}

static int ambarella_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

	if(prtd){
		if(prtd->dma_desc_array != NULL){
			dma_free_coherent(substream->pcm->card->dev,
						AMBA_MAX_DESC_NUM * sizeof(ambarella_dma_req_t),
						prtd->dma_desc_array, prtd->dma_desc_array_phys);
			prtd->dma_desc_array = NULL;
		}

		if(prtd->dma_rpt_buf != NULL) {
			dma_free_coherent(substream->pcm->card->dev,
				AMBA_MAX_DESC_NUM * sizeof(prtd->dma_rpt_buf),
				prtd->dma_rpt_buf, prtd->dma_rpt_phys);
			prtd->dma_rpt_buf = NULL;
		}

		kfree(prtd);
		runtime->private_data = NULL;
	}

	return 0;
}

static int ambarella_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	return dma_mmap_coherent(substream->pcm->card->dev, vma,
		runtime->dma_area, runtime->dma_addr, runtime->dma_bytes);
}

static struct snd_pcm_ops ambarella_pcm_ops = {
	.open		= ambarella_pcm_open,
	.close		= ambarella_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= ambarella_pcm_hw_params,
	.hw_free	= ambarella_pcm_hw_free,
	.prepare	= ambarella_pcm_prepare,
	.trigger	= ambarella_pcm_trigger,
	.pointer	= ambarella_pcm_pointer,
	.mmap		= ambarella_pcm_mmap,
};

static int ambarella_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = ambarella_pcm_hardware.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	return 0;
}

static void ambarella_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream <= SNDRV_PCM_STREAM_LAST; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
			buf->area, buf->addr);
		buf->area = NULL;
		buf->addr = (dma_addr_t)NULL;
	}
}

static int ambarella_pcm_new(struct snd_card *card,
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;

	card->dev->dma_mask = &ambarella_dmamask;
	card->dev->coherent_dma_mask = ambarella_dmamask;

	if (dai->driver->playback.channels_min) {
		ret = ambarella_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (dai->driver->capture.channels_min) {
		ret = ambarella_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
out:
	return ret;
}

static struct snd_soc_platform_driver ambarella_soc_platform = {
	.pcm_new	= ambarella_pcm_new,
	.pcm_free	= ambarella_pcm_free_dma_buffers,
	.ops		= &ambarella_pcm_ops,
};

static int __devinit ambarella_soc_platform_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &ambarella_soc_platform);
}

static int __devexit ambarella_soc_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver ambarella_pcm_driver = {
	.driver = {
			.name = "ambarella-pcm-audio",
			.owner = THIS_MODULE,
	},

	.probe = ambarella_soc_platform_probe,
	.remove = __devexit_p(ambarella_soc_platform_remove),
};

static int __init snd_ambarella_pcm_init(void)
{
	return platform_driver_register(&ambarella_pcm_driver);
}
module_init(snd_ambarella_pcm_init);

static void __exit snd_ambarella_pcm_exit(void)
{
	platform_driver_unregister(&ambarella_pcm_driver);
}
module_exit(snd_ambarella_pcm_exit);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc PCM DMA module");
MODULE_LICENSE("GPL");

