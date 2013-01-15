/*
 * sound/soc/a5spa2.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * History:
 *	2009/08/20 - [Cao Rongrong] Created file
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38,
 *				    merge coconut and durian into a5sevk
 *	2011/05/11 - [Louis Sun] modify it for PA2 board
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

#include <linux/platform_device.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <mach/hardware.h>
#include <plat/audio.h>

#include "../codecs/wm8737.h"

static unsigned int dai_fmt = 0;
module_param(dai_fmt, uint, 0644);
MODULE_PARM_DESC(dai_fmt, "DAI format.");

#define WM8737_SYSCLK  0

static int a5sevk_board_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int a5sevk_board_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int errorCode = 0, amb_mclk, mclk, oversample, i2s_mode;

	switch (params_rate(params)) {
	case 8000:
		amb_mclk = AudioCodec_2_048M;
		mclk = 2048000;
		oversample = AudioCodec_256xfs;
		break;
	case 11025:
		amb_mclk = AudioCodec_2_8224M;
		mclk = 2822400;
		oversample = AudioCodec_256xfs;
		break;
	case 12000:
		amb_mclk = AudioCodec_3_072M;
		mclk = 3072000;
		oversample = AudioCodec_256xfs;
		break;
	case 16000:
		amb_mclk = AudioCodec_4_096M;
		mclk = 4096000;
		oversample = AudioCodec_256xfs;
		break;
	case 22050:
		amb_mclk = AudioCodec_5_6448M;
		mclk = 5644800;
		oversample = AudioCodec_256xfs;
		break;
	case 24000:
		amb_mclk = AudioCodec_6_144;
		mclk = 6144000;
		oversample = AudioCodec_256xfs;
		break;
	case 32000:
		amb_mclk = AudioCodec_8_192M;
		mclk = 8192000;
		oversample = AudioCodec_256xfs;
		break;
	case 44100:
		amb_mclk = AudioCodec_11_2896M;
		mclk = 11289600;
		oversample = AudioCodec_256xfs;
		break;
	case 48000:
		amb_mclk = AudioCodec_12_288M;
		mclk = 12288000;
		oversample = AudioCodec_256xfs;
		break;
	default:
		errorCode = -EINVAL;
		goto hw_params_exit;
	}

	if (dai_fmt == 0)
		i2s_mode = SND_SOC_DAIFMT_I2S;
	else
		i2s_mode = SND_SOC_DAIFMT_DSP_A;

	/* set the I2S system data format*/
	errorCode = snd_soc_dai_set_fmt(codec_dai,
		i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set codec DAI configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_fmt(cpu_dai,
		i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set cpu DAI configuration\n");
		goto hw_params_exit;
	}

	/* set the I2S system clock*/
	errorCode = snd_soc_dai_set_sysclk(codec_dai, WM8737_SYSCLK, mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_sysclk(cpu_dai, AMBARELLA_CLKSRC_ONCHIP, amb_mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_clkdiv(cpu_dai, AMBARELLA_CLKDIV_LRCLK, oversample);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK/SF ratio\n");
		goto hw_params_exit;
	}

hw_params_exit:
	return errorCode;
}

static struct snd_soc_ops a5sevk_board_ops = {
	.startup = a5sevk_board_startup,
	.hw_params = a5sevk_board_hw_params,
};

/* a5sevk machine dapm widgets */
static const struct snd_soc_dapm_widget a5sevk_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Internal MIC", NULL),
	SND_SOC_DAPM_MIC("External MIC", NULL),
};

/* a5sevk machine audio map (connections to ak4642 pins) */
static const struct snd_soc_dapm_route a5sevk_audio_map[] = {
	/* Line In is connected to LLIN1, RLIN1 */
	{"LINPUT1", NULL, "Internal MIC"},	//internal MIC
	{"RINPUT1", NULL, "Internal MIC"},	//internal MIC
	{"LINPUT2", NULL, "External MIC"},	//external MIC
	{"RINPUT2", NULL, "External MIC"},	//external MIC

};

static int a5sevk_wm8737_init(struct snd_soc_pcm_runtime *rtd)
{
	int errorCode = 0;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	/* Add a5sevk specific widgets */
	errorCode = snd_soc_dapm_new_controls(dapm,
		a5sevk_dapm_widgets,
		ARRAY_SIZE(a5sevk_dapm_widgets));
	if (errorCode) {
		goto init_exit;
	}

	/* Set up a5sevk specific audio path a5sevk_audio_map */
	errorCode = snd_soc_dapm_add_routes(dapm,
		a5sevk_audio_map,
		ARRAY_SIZE(a5sevk_audio_map));
	if (errorCode) {
		goto init_exit;
	}
	
	errorCode = snd_soc_dapm_sync(dapm);

init_exit:
	return errorCode;
}

/* a5sevk digital audio interface glue - connects codec <--> A2S */
static struct snd_soc_dai_link a5sevk_dai_link = {
	.name = "WM8737",
	.stream_name = "WM8737-STREAM",
	.cpu_dai_name = "ambarella-i2s.0",
	.platform_name = "ambarella-pcm-audio",
	.codec_dai_name = "wm8737",
	.codec_name = "wm8737.1-001a",
	.init = a5sevk_wm8737_init,
	.ops = &a5sevk_board_ops,
};


/* a5sevk audio machine driver */
static struct snd_soc_card snd_soc_card_a5sevk = {
	.name = "A5S-PA2",
	.dai_link = &a5sevk_dai_link,
	.num_links = 1,
};

static struct platform_device *a5sevk_snd_device;

static int __init a5sevk_board_init(void)
{
	int errorCode = 0;

	a5sevk_snd_device = platform_device_alloc("soc-audio", -1);
	if (!a5sevk_snd_device)
		return -ENOMEM;

	platform_set_drvdata(a5sevk_snd_device, &snd_soc_card_a5sevk);

	errorCode = platform_device_add(a5sevk_snd_device);

	if (errorCode) {
		goto a5sevk_board_init_exit;
	}

	return 0;

a5sevk_board_init_exit:
	platform_device_put(a5sevk_snd_device);
	return errorCode;
}

static void __exit a5sevk_board_exit(void)
{
	platform_device_unregister(a5sevk_snd_device);
}

module_init(a5sevk_board_init);
module_exit(a5sevk_board_exit);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella A5s PA2 Board with WM8737 Codec for ALSA");
MODULE_LICENSE("GPL");
MODULE_ALIAS("snd-soc-coconut");

