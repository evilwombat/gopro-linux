/*
 * sound/soc/ambarella_i2s.c
 *
 * History:
 *	2008/03/03 - [Eric Lee] created file
 *	2008/04/16 - [Eric Lee] Removed the compiling warning
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *	2009/03/05 - [Cao Rongrong] Update from 2.6.22.10
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
 *	2009/06/29 - [Cao Rongrong] Support more mclk and fs
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <mach/hardware.h>
#include <plat/audio.h>

#include "ambarella_pcm.h"
#include "ambarella_i2s.h"

/*
  * The I2S ports are mux with the GPIOs.
  * A3, A5, A5S and A6 support 5.1(6) channels, so we need to
  * select the proper port to used and free the unused pins (GPIOs)
  * for other usage.
  * 1: 2 channels
  * 2: 4 channels
  * 3: 6 channels
  */
unsigned int used_port = 1;
module_param(used_port, uint, S_IRUGO);
MODULE_PARM_DESC(used_port, "Select the I2S port.");

static u32 DAI_Clock_Divide_Table[MAX_OVERSAMPLE_IDX_NUM][2] = {
	{ 1, 0 }, // 128xfs
	{ 3, 1 }, // 256xfs
	{ 5, 2 }, // 384xfs
	{ 7, 3 }, // 512xfs
	{ 11, 5 }, // 768xfs
	{ 15, 7 }, // 1024xfs
	{ 17, 8 }, // 1152xfs
	{ 23, 11 }, // 1536xfs
	{ 35, 17 } // 2304xfs
};


/* FIXME HERE for PCM interface */
static struct ambarella_pcm_dma_params ambarella_i2s_pcm_stereo_out = {
	.name			= "I2S PCM Stereo out",
	.dev_addr		= I2S_TX_LEFT_DATA_DMA_REG,
};

static struct ambarella_pcm_dma_params ambarella_i2s_pcm_stereo_in = {
	.name			= "I2S PCM Stereo in",
	.dev_addr		= I2S_RX_DATA_DMA_REG,
};

static inline void dai_tx_enable(void)
{
	amba_setbitsl(I2S_INIT_REG, DAI_TX_EN);
}

static inline void dai_rx_enable(void)
{
	amba_setbitsl(I2S_INIT_REG, DAI_RX_EN);
}

static inline void dai_tx_disable(void)
{
	amba_clrbitsl(I2S_INIT_REG, DAI_TX_EN);
}

static inline void dai_rx_disable(void)
{
	amba_clrbitsl(I2S_INIT_REG, DAI_RX_EN);
}

static inline void dai_fifo_rst(void)
{
	amba_setbitsl(I2S_INIT_REG, DAI_FIFO_RST);
	msleep(1);
	if (amba_tstbitsl(I2S_INIT_REG, DAI_FIFO_RST)) {
		printk("DAI_FIFO_RST fail!\n");
	}
}

static int ambarella_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *cpu_dai)
{
	struct ambarella_pcm_dma_params *dma_data;
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);
	u8 slots, word_pos, clksrc, mclk, oversample;
	u8 rx_enabled = 0, tx_enabled = 0;
	u32 clock_divider, channels;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data = &ambarella_i2s_pcm_stereo_out;
		/* Disable tx/rx before initializing */
		dai_tx_disable();
		if(amba_tstbitsl(I2S_INIT_REG, 0x02)) {
			rx_enabled = 1;
			dai_rx_disable();
		}
		amba_writel(I2S_TX_CTRL_REG, 0x28);
		amba_writel(I2S_TX_FIFO_LTH_REG, 0x10);
	} else {
		dma_data = &ambarella_i2s_pcm_stereo_in;
		/* Disable tx/rx before initializing */
		dai_rx_disable();
		if(amba_tstbitsl(I2S_INIT_REG, 0x04)) {
			tx_enabled = 1;
			dai_tx_disable();
		}
		amba_writel(I2S_RX_CTRL_REG, 0x02);
		amba_writel(I2S_RX_FIFO_GTH_REG, 0x40);
	}

	snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

	/* Set channels */
	channels = params_channels(params);
	if (priv_data->controller_info->channel_select)
		priv_data->controller_info->channel_select(channels);
	priv_data->amb_i2s_intf.ch = channels;

	/* Set format */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (priv_data->amb_i2s_intf.mode == DAI_DSP_Mode) {
			slots = channels - 1;
			word_pos = 0x0f;
			priv_data->amb_i2s_intf.slots = slots;
		} else {
			slots = 0;
			word_pos = 0;
			priv_data->amb_i2s_intf.slots = DAI_32slots;
		}
		priv_data->amb_i2s_intf.word_len = DAI_16bits;
		priv_data->amb_i2s_intf.word_pos = word_pos;
		priv_data->amb_i2s_intf.word_order = DAI_MSB_FIRST;

		amba_writel(I2S_MODE_REG, priv_data->amb_i2s_intf.mode);
		amba_writel(I2S_WLEN_REG, 0x0f);
		amba_writel(I2S_WPOS_REG, word_pos);
		amba_writel(I2S_SLOT_REG, slots);
		amba_writel(I2S_24BITMUX_MODE_REG, 0);

		break;
	default:
		return -EINVAL;
	}

	switch (params_rate(params)) {
	case 8000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_48000;
		break;
	case 11025:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_11025;
		break;
	case 16000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_16000;
		break;
	case 22050:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_22050;
		break;
	case 32000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_32000;
		break;
	case 44100:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_44100;
		break;
	case 48000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_48000;
		break;
	default:
		return -EINVAL;
	}

	/* Set clock */
	clksrc = priv_data->amb_i2s_intf.clksrc;
	mclk = priv_data->amb_i2s_intf.mclk;
	oversample = priv_data->amb_i2s_intf.oversample;
	if (priv_data->controller_info->set_audio_pll)
		priv_data->controller_info->set_audio_pll(clksrc, mclk);
	clock_divider = DAI_Clock_Divide_Table[oversample][slots >> 6];
	clock_divider |= 0x3C0 ;
	priv_data->clock_reg &= (u16)DAI_CLOCK_MASK;
	priv_data->clock_reg |= clock_divider;
	amba_writel(I2S_CLOCK_REG, priv_data->clock_reg);

	if(!amba_tstbitsl(I2S_INIT_REG, 0x6))
		dai_fifo_rst();

	if(rx_enabled)
		dai_rx_enable();
	if(tx_enabled)
		dai_tx_enable();

	msleep(1);

	/* Notify HDMI that the audio interface is changed */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_SETHWPARAMS);

	return 0;
}

static int ambarella_i2s_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	if(substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		dai_fifo_rst();
	return 0;
}

static int ambarella_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
			struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			dai_rx_enable();
		} else {
			if(alsa_tx_enable_flag == 0)
				dai_tx_enable();
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			//dai_rx_disable();
			//Stop by DMA EOC
		} else {
			//dai_tx_disable();
			//Stop by DMA EOC
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*
 * Set Ambarella I2S DAI format
 */
static int ambarella_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_LEFT_J:
		priv_data->amb_i2s_intf.mode = DAI_leftJustified_Mode;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		priv_data->amb_i2s_intf.mode = DAI_rightJustified_Mode;
		break;
	case SND_SOC_DAIFMT_I2S:
		priv_data->amb_i2s_intf.mode = DAI_I2S_Mode;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		priv_data->amb_i2s_intf.mode = DAI_DSP_Mode;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (clk_id) {
	case AMBARELLA_CLKSRC_ONCHIP:
	case AMBARELLA_CLKSRC_EXTERNAL:
		priv_data->amb_i2s_intf.clksrc = clk_id;
		priv_data->amb_i2s_intf.mclk = freq;
		break;
	default:
		printk("CLK SOURCE (%d) is not supported yet\n", clk_id);
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
		int div_id, int div)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (div_id) {
	case AMBARELLA_CLKDIV_LRCLK:
		priv_data->amb_i2s_intf.oversample = div;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_dai_probe(struct snd_soc_dai *dai)
{
	u32 clock_divider;
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	/* Switch to external I2S Input except  IPcam Board */
	if (strcmp(dai->card->name, "A2IPcam"))
		priv_data->clock_reg = 0x1<<10;

	if (priv_data->controller_info->set_audio_pll)
		priv_data->controller_info->set_audio_pll(AMBARELLA_CLKSRC_ONCHIP, AudioCodec_11_2896M);

	/* Dai default smapling rate, polarity configuration*/
	clock_divider = DAI_Clock_Divide_Table[AudioCodec_256xfs][DAI_32slots >> 6];
	clock_divider |= 0x3C0 ;
	priv_data->clock_reg &= (u16)DAI_CLOCK_MASK;
	priv_data->clock_reg |= clock_divider;
	amba_writel(I2S_CLOCK_REG, priv_data->clock_reg);

	priv_data->amb_i2s_intf.mode = DAI_I2S_Mode;
	priv_data->amb_i2s_intf.clksrc = AMBARELLA_CLKSRC_ONCHIP;
	priv_data->amb_i2s_intf.mclk = AudioCodec_11_2896M;
	priv_data->amb_i2s_intf.oversample = AudioCodec_256xfs;
	priv_data->amb_i2s_intf.word_order = DAI_MSB_FIRST;
	priv_data->amb_i2s_intf.sfreq = AUDIO_SF_44100;
	priv_data->amb_i2s_intf.word_len = DAI_16bits;
	priv_data->amb_i2s_intf.word_pos = 0;
	priv_data->amb_i2s_intf.slots = DAI_32slots;
	priv_data->amb_i2s_intf.ch = 2;

	/* Notify HDMI that the audio interface is initialized */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf, AUDIO_NOTIFY_INIT);

	return 0;
}

static int ambarella_i2s_dai_remove(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	/* Notify that the audio interface is removed */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_REMOVE);

	return 0;
}

static struct snd_soc_dai_ops ambarella_i2s_dai_ops = {
		.prepare = ambarella_i2s_prepare,
		.trigger = ambarella_i2s_trigger,
		.hw_params = ambarella_i2s_hw_params,
		.set_fmt = ambarella_i2s_set_fmt,
		.set_sysclk = ambarella_i2s_set_sysclk,
		.set_clkdiv = ambarella_i2s_set_clkdiv,
};

static struct snd_soc_dai_driver ambarella_i2s_dai = {
	.probe = ambarella_i2s_dai_probe,
	.remove = ambarella_i2s_dai_remove,
	.playback = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &ambarella_i2s_dai_ops,
	.symmetric_rates = 1,
};

static int __devinit ambarella_i2s_probe(struct platform_device *pdev)
{
	struct amb_i2s_priv *priv_data;

	priv_data = kzalloc(sizeof(struct amb_i2s_priv), GFP_KERNEL);
	if (priv_data == NULL)
		return -ENOMEM;

	priv_data->controller_info = pdev->dev.platform_data;
	/* aucodec_digitalio_on */
	switch(used_port) {
	case 3:
		ambarella_i2s_dai.playback.channels_max += 2;
		ambarella_i2s_dai.capture.channels_max += 2;
		if (priv_data->controller_info->aucodec_digitalio_2)
			priv_data->controller_info->aucodec_digitalio_2();

	case 2:
		ambarella_i2s_dai.playback.channels_max += 2;
		ambarella_i2s_dai.capture.channels_max += 2;
		if (priv_data->controller_info->aucodec_digitalio_1)
			priv_data->controller_info->aucodec_digitalio_1();

	case 1:
		ambarella_i2s_dai.playback.channels_max += 2;
		ambarella_i2s_dai.capture.channels_max += 2;
		if (priv_data->controller_info->aucodec_digitalio_0)
			priv_data->controller_info->aucodec_digitalio_0();
		break;

	default:
		printk("%s: Need to select proper I2S port.\n", __func__);
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, priv_data);

	return snd_soc_register_dai(&pdev->dev, &ambarella_i2s_dai);
}

static int __devexit ambarella_i2s_remove(struct platform_device *pdev)
{
	struct amb_i2s_priv *priv_data = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_dai(&pdev->dev);
	kfree(priv_data);

	return 0;
}

static struct platform_driver ambarella_i2s_driver = {
	.probe = ambarella_i2s_probe,
	.remove = __devexit_p(ambarella_i2s_remove),

	.driver = {
		.name = "ambarella-i2s",
		.owner = THIS_MODULE,
	},
};


static int __init ambarella_i2s_init(void)
{
	return platform_driver_register(&ambarella_i2s_driver);
}
module_init(ambarella_i2s_init);

static void __exit ambarella_i2s_exit(void)
{
	platform_driver_unregister(&ambarella_i2s_driver);
}
module_exit(ambarella_i2s_exit);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc I2S Interface");

MODULE_LICENSE("GPL");

