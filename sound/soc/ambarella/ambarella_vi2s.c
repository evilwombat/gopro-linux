/*
 * sound/soc/ambarella_vi2s.c
 *
 * History:
 *	2011/03/28 - [Eric Lee] Port from ambarella_i2s.c
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

#include "ambarella_vpcm.h"
#include "ambarella_vi2s.h"

//#define ALSA_VI2S_DEBUG
#ifdef ALSA_VI2S_DEBUG
#define vi2s_printk	printk
#else
#define vi2s_printk(...)
#endif

extern int ipc_ialsa_get_max_channels(void);

/*
  * The I2S ports are mux with the GPIOs.
  * A3, A5, A5S and A6 support 5.1(6) channels, so we need to
  * select the proper port to used and free the unused pins (GPIOs)
  * for other usage.
  * 1: 2 channels
  * 2: 4 channels
  * 3: 6 channels
  */
unsigned int used_port;
unsigned int op_port = 2;
unsigned int op_sfreq = 48000;
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

static int ambarella_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *cpu_dai)
{
	struct ambarella_pcm_dma_params *dma_data;
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);
	u8 slots, word_pos, oversample;
	u32 clock_divider, channels;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data = &ambarella_i2s_pcm_stereo_out;
	} else {
		dma_data = &ambarella_i2s_pcm_stereo_in;
	}

	snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

	/* Set channels */
	channels = params_channels(params);
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
	oversample = priv_data->amb_i2s_intf.oversample;
  clock_divider = DAI_Clock_Divide_Table[oversample][slots >> 6];
	clock_divider |= 0x3C0;
	priv_data->clock_reg &= (u16)DAI_CLOCK_MASK;
	priv_data->clock_reg |= clock_divider;
	/* Notify HDMI that the audio interface is changed */
	/* FIXME: Do we need this in dual OS? */
	//ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
	//	AUDIO_NOTIFY_SETHWPARAMS);

  op_sfreq = params_rate(params);
	op_port = channels;
  	vi2s_printk("ambarella_i2s_hw_params ch: %d, freq: %d\n",
		channels, params_rate(params));

	return 0;
}

static int ambarella_i2s_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
  vi2s_printk("ambarella_i2s_prepare\n");
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
		//Controlled by PCM
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		//Controlled by PCM
		break;
	default:
		ret = -EINVAL;
		break;
	}

  vi2s_printk("ambarella_i2s_trigger\n");
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
	/* Only support I2S mode in virtual driver currently */
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

  vi2s_printk("ambarella_i2s_set_fmt %d\n", fmt);
	return 0;
}

static int ambarella_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (clk_id) {
	case AMBARELLA_CLKSRC_ONCHIP:
		priv_data->amb_i2s_intf.mclk = freq;
		break;
	default:
		printk("CLK SOURCE (%d) is not supported yet\n", clk_id);
		return -EINVAL;
	}

  vi2s_printk("ambarella_i2s_set_sysclk %d\n", clk_id);
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

  vi2s_printk("ambarella_i2s_set_clkdiv %d\n", div);
	return 0;
}

static int ambarella_i2s_dai_probe(struct snd_soc_dai *dai)
{
	u32 clock_divider;
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	/* Dai default smapling rate, polarity configuration*/
	clock_divider = DAI_Clock_Divide_Table[AudioCodec_256xfs][DAI_32slots >> 6];
	clock_divider |= 0x3C0 ;
	priv_data->clock_reg &= (u16)DAI_CLOCK_MASK;
	priv_data->clock_reg |= clock_divider;
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
	/* FIXME: Do we need this in dual OS? */
	//ambarella_audio_notify_transition(&priv_data->amb_i2s_intf, AUDIO_NOTIFY_INIT);

  vi2s_printk("ambarella_i2s_dai_probe\n");
	return 0;
}

static int ambarella_i2s_dai_remove(struct snd_soc_dai *dai)
{
	//struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	/* Notify that the audio interface is removed */
	// FIXME check this with Rongrong
	/* ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_REMOVE); */
	vi2s_printk("ambarella_i2s_dai_remove\n");
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
		.rates = (SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = (SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &ambarella_i2s_dai_ops,
};

static int __devinit ambarella_i2s_probe(struct platform_device *pdev)
{
	struct amb_i2s_priv *priv_data;

	priv_data = kzalloc(sizeof(struct amb_i2s_priv), GFP_KERNEL);
	if (priv_data == NULL)
		return -ENOMEM;

	priv_data->controller_info = pdev->dev.platform_data;
	/* aucodec_digitalio_on */

	used_port = ipc_ialsa_get_max_channels();
	  
	ambarella_i2s_dai.playback.channels_max = used_port;
	ambarella_i2s_dai.capture.channels_max = used_port;

	dev_set_drvdata(&pdev->dev, priv_data);
	
	vi2s_printk("ambarella_i2s_probe\n");

	return snd_soc_register_dai(&pdev->dev, &ambarella_i2s_dai);
}

static int __devexit ambarella_i2s_remove(struct platform_device *pdev)
{
	struct amb_i2s_priv *priv_data = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_dai(&pdev->dev);
	kfree(priv_data);

	vi2s_printk("ambarella_i2s_remove\n");
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

MODULE_AUTHOR("Eric Lee <cylee@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc Virtual I2S Interface");

MODULE_LICENSE("GPL");

