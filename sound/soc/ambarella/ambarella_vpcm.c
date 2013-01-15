/*
 * sound/soc/ambarella_vpcm.c
 *
 * History:
 *  2011/03/28 - [Eric Lee] Port from ambarella_pcm.c
 *	2011/06/23 - [Eric Lee] Port to 2.6.38
 *
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

#include "ambarella_vpcm.h"

extern unsigned int op_port;
extern unsigned int op_sfreq;
static unsigned int op_addr;
static unsigned int op_size;

#define AMBA_MAX_DESC_NUM   32
#define AMBA_MIN_DESC_NUM   4
#define AMBA_PERIOD_BYTES_MAX		8192
#define AMBA_PERIOD_BYTES_MIN   256 * 2 * 2

//#define ALSA_VPCM_DEBUG
#ifdef ALSA_VPCM_DEBUG
#define vpcm_printk	printk
#else
#define vpcm_printk(...)
#endif

extern int ipc_ialsa_tx_open(unsigned int ch, unsigned int freq,
  unsigned int base, int size, int desc_num);
extern int ipc_ialsa_tx_start(void);
extern int ipc_ialsa_tx_stop(void);
extern int ipc_ialsa_tx_close(void);
extern int ipc_ialsa_rx_open(unsigned int ch, unsigned int freq,
  unsigned int base, int size, int desc_num);
extern int ipc_ialsa_rx_start(void);
extern int ipc_ialsa_rx_stop(void);
extern int ipc_ialsa_rx_close(void);

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
	int op_stop;
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
	.buffer_bytes_max	= AMBA_MAX_DESC_NUM * AMBA_PERIOD_BYTES_MAX,
};

static void v_daidma_fifo_update(void *dev_id)
{
	struct snd_pcm_substream *substream = dev_id;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

	spin_lock_irq(&prtd->lock);
	prtd->last_descr++;
	if (prtd->last_descr >= prtd->ndescr) {
		prtd->last_descr = 0;
	}
	spin_unlock_irq(&prtd->lock);
	snd_pcm_period_elapsed(substream);

	//printk("v_daidma_fifo_update %d\n", prtd->last_descr);
}

static void alsa_op_finish(void *dev_id)
{
	struct snd_pcm_substream *substream = dev_id;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

  prtd->working = 0;
  wake_up(&prtd->wq);
}

extern int vdma_request_callback(int chan, void (*handler1)(void *),
  void (*handler2)(void *), void *harg);
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
	int ret;

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
		ret = vdma_request_callback(0, v_daidma_fifo_update, alsa_op_finish,
      substream);
		if (ret < 0)
			return ret;
	} else {
		ret = vdma_request_callback(1, v_daidma_fifo_update, alsa_op_finish,
      substream);
		if (ret < 0)
			return ret;
	}

  op_addr = runtime->dma_addr;
  op_size = period;

	prtd->ndescr = 0;
	prtd->last_descr = 0;
	do {
		if (period > totsize)
			period = totsize;
		prtd->ndescr++;
	} while (totsize -= period);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
    	ipc_ialsa_tx_open(op_port, op_sfreq, op_addr, op_size, prtd->ndescr);
	} else {
    	ipc_ialsa_rx_open(op_port, op_sfreq, op_addr, op_size, prtd->ndescr);
	}

  vpcm_printk("ambarella_pcm_hw_params\n");
  vpcm_printk("period: %d, ndescr: %d dma adr: 0x%x dma area: 0x%x\n",
		period, prtd->ndescr, runtime->dma_addr, (unsigned int)runtime->dma_area);

	return 0;
}

static int ambarella_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;

	if (prtd->dma_data) {
		wait_event_timeout(prtd->wq, (prtd->working == 0), 3 * HZ);
		/* Disable and free DMA irq */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/* unregister call back ? */
		} else {
			/* unregister call back ? */
		}
		prtd->dma_data = NULL;
		prtd->ndescr = 0;
		prtd->last_descr = 0;
	}

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	vpcm_printk("ambarella_pcm_hw_free\n");

	return 0;
}

static int ambarella_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;

	vpcm_printk("ambarella_pcm_prepare\n");

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
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ipc_ialsa_tx_start();
		} else {
			ipc_ialsa_rx_start();
		}
		prtd->working = 1;
		prtd->op_stop = 0;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ipc_ialsa_tx_stop();
		} else {
			ipc_ialsa_rx_stop();
		}
		prtd->op_stop = 1;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock_irq(&prtd->lock);
	
	vpcm_printk("ambarella_pcm_trigger %d\n", cmd);

	return ret;
}

static snd_pcm_uframes_t ambarella_pcm_pointer(
	struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ambarella_runtime_data *prtd = runtime->private_data;
	snd_pcm_uframes_t x;

	spin_lock_irq(&prtd->lock);
	x = prtd->last_descr * runtime->period_size;
	spin_unlock_irq(&prtd->lock);

	return x;
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
	prtd->op_stop = 0;

	runtime->private_data = prtd;
	goto ambarella_pcm_open_exit;

ambarella_pcm_open_free_dma_desc_array:
	dma_free_coherent(substream->pcm->card->dev,
		AMBA_MAX_DESC_NUM * sizeof(ambarella_dma_req_t),
		prtd->dma_desc_array, prtd->dma_desc_array_phys);

ambarella_pcm_open_free_prtd:
	kfree(prtd);

ambarella_pcm_open_exit:
	vpcm_printk("ambarella_pcm_open\n");

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
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ipc_ialsa_tx_close();
	} else {
		ipc_ialsa_rx_close();
	}
	
	vpcm_printk("ambarella_pcm_close\n");
	
	return 0;
}

static int ambarella_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	vpcm_printk("ambarella_pcm_mmap\n");

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
	
	vpcm_printk("ambarella_pcm_preallocate_dma_buffer: 0x%x %d\n",
		(unsigned int)buf->addr, size);

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

	vpcm_printk("ambarella_pcm_free_dma_buffers\n");
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

	vpcm_printk("ambarella_pcm_new\n");

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

MODULE_AUTHOR("Eric Lee <cylee@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc Virtual PCM DMA module");
MODULE_LICENSE("GPL");

