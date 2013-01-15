/*
 * sound/soc/i1sevk.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * History:
 *	2011/03/28 - [Cao Rongrong] Created file
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
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <asm/mach-types.h>
#include <mach/gpio.h>
#include <plat/audio.h>

#include <linux/mfd/wm8994/core.h>
#include <linux/mfd/wm8994/registers.h>
#include "../codecs/wm8994.h"

/* If don't use alsa standard Jack detction mechanism,
 * switch class will take over this job. */
static unsigned int gpio_jack = 1;
module_param(gpio_jack, uint, S_IRUGO);
MODULE_PARM_DESC(gpio_jack, "Whether or not use alsa standard Jack detection mechanism.");

/* Headset jack */
static struct snd_soc_jack hs_jack;

/* Headset jack detection DAPM pins */
static struct snd_soc_jack_pin hs_jack_pins[] = {
	{
		.pin = "Headset Mic",
		.mask = SND_JACK_MICROPHONE,
	},
	{
		.pin = "Headset Stereophone",
		.mask = SND_JACK_HEADPHONE,
	},
};

/* Headset jack detection gpios */
static struct snd_soc_jack_gpio hs_jack_gpios[] = {
	{
		.gpio = GPIO(12),
		.name = "hsdet-gpio",
		.report = SND_JACK_HEADSET,
		.debounce_time = 200,
	},
};

/* AV jack */
static struct snd_soc_jack av_jack;

/* AV jack detection DAPM pins */
static struct snd_soc_jack_pin av_jack_pins[] = {
	{
		.pin = "Line Out 2",
		.mask = SND_JACK_LINEOUT,
	},
};

/* AV jack detection gpios */
static struct snd_soc_jack_gpio av_jack_gpios[] = {
	{
		.gpio = GPIO(13),
		.name = "avdet-gpio",
		.report = SND_JACK_LINEOUT,
		.debounce_time = 200,
	},
};

static struct gpio_switch_platform_data headset_switch_data = {
       .name = "h2w",
       .gpio = GPIO(12),
};

static struct platform_device headset_switch_device = {
       .name             = "switch-gpio",
       .dev = {
               .platform_data    = &headset_switch_data,
       }
};

static const struct snd_soc_dapm_widget i1evk_dapm_widgets[] = {
	/* Output */
	SND_SOC_DAPM_SPK("Ext Left Spk", NULL),
	SND_SOC_DAPM_SPK("Ext Right Spk", NULL),
	SND_SOC_DAPM_SPK("Earphone", NULL),
	SND_SOC_DAPM_HP("Headset Stereophone", NULL),
	SND_SOC_DAPM_LINE("Line Out 1", NULL),
	SND_SOC_DAPM_LINE("Line Out 2", NULL),
	/* Input */
	SND_SOC_DAPM_MIC("Main Mic", NULL),
	SND_SOC_DAPM_MIC("2nd Mic", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_LINE("FM Left In", NULL),
	SND_SOC_DAPM_LINE("FM Right In", NULL),
	SND_SOC_DAPM_LINE("3G In", NULL),
};

static const struct snd_soc_dapm_route i1evk_dapm_routes[] = {
	{"Ext Left Spk", NULL, "SPKOUTLP"},
	{"Ext Left Spk", NULL, "SPKOUTLN"},

	{"Ext Right Spk", NULL, "SPKOUTRP"},
	{"Ext Right Spk", NULL, "SPKOUTRN"},

	{"Earphone", NULL, "HPOUT2N"},
	{"Earphone", NULL, "HPOUT2P"},

	{"Headset Stereophone", NULL, "HPOUT1L"},
	{"Headset Stereophone", NULL, "HPOUT1R"},

	{"Line Out 1", NULL, "LINEOUT1N"},
	{"Line Out 1", NULL, "LINEOUT1P"},

	{"Line Out 2", NULL, "LINEOUT2N"},
	{"Line Out 2", NULL, "LINEOUT2P"},

	{"IN1LN", NULL, "MICBIAS1"},
	{"MICBIAS1", NULL, "Main Mic"},

	{"IN1LP", NULL, "FM Left In"},

	{"IN2LN", NULL, "MICBIAS2"},
	{"MICBIAS2", NULL, "Headset Mic"},

	{"IN1RN", NULL, "MICBIAS1"},
	{"MICBIAS1", NULL, "2nd Mic"},

	{"IN1RP", NULL, "FM Right In"},

	{"IN2LP:VXRN", NULL, "3G In"},
	{"IN2RP:VXRP", NULL, "3G In"},
};

static int i1evk_wm8994_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int errorCode = 0;

	/* add i1evk specific widgets */
	snd_soc_dapm_new_controls(dapm, i1evk_dapm_widgets,
			ARRAY_SIZE(i1evk_dapm_widgets));

	/* set up i1evk specific audio routes */
	snd_soc_dapm_add_routes(dapm, i1evk_dapm_routes,
			ARRAY_SIZE(i1evk_dapm_routes));

	snd_soc_dapm_sync(dapm);

	if (gpio_jack) {
		/* Headset jack detection */
		errorCode = snd_soc_jack_new(codec,
				"Headset Jack", SND_JACK_HEADSET, &hs_jack);
		if (errorCode)
			return errorCode;

		errorCode = snd_soc_jack_add_pins(&hs_jack,
				ARRAY_SIZE(hs_jack_pins), hs_jack_pins);
		if (errorCode)
			return errorCode;

		errorCode = snd_soc_jack_add_gpios(&hs_jack,
				ARRAY_SIZE(hs_jack_gpios), hs_jack_gpios);
		if (errorCode)
			return errorCode;

		/* Headset jack detection */
		errorCode = snd_soc_jack_new(codec,
				"AV Jack", SND_JACK_LINEOUT, &av_jack);
		if (errorCode)
			return errorCode;

		errorCode = snd_soc_jack_add_pins(&av_jack,
				ARRAY_SIZE(av_jack_pins), av_jack_pins);
		if (errorCode)
			return errorCode;

		errorCode = snd_soc_jack_add_gpios(&av_jack,
				ARRAY_SIZE(av_jack_gpios), av_jack_gpios);
	} else {
		platform_device_register(&headset_switch_device);
	}

	return errorCode;
}

static int i1evk_hifi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int errorCode = 0, amb_mclk, mclk, oversample;

	switch (params_rate(params)) {
	case 8000:
		amb_mclk = AudioCodec_4_096M;
		mclk = 4096000;
		oversample = AudioCodec_512xfs;
		break;
	case 11025:
		amb_mclk = AudioCodec_5_6448M;
		mclk = 5644800;
		oversample = AudioCodec_512xfs;
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

	/* set the I2S system data format*/
	errorCode = snd_soc_dai_set_fmt(codec_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set codec DAI configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_fmt(cpu_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set cpu DAI configuration\n");
		goto hw_params_exit;
	}

	/* set the I2S system clock*/
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
#if 0
	errorCode = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, WM8994_FLL_SRC_MCLK1,
					mclk, 11289600);
	if (errorCode < 0) {
		pr_err("can't set codec pll configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
					11289600, SND_SOC_CLOCK_IN);
	if (errorCode < 0) {
		pr_err("can't set codec MCLK configuration\n");
		goto hw_params_exit;
	}
#else
	errorCode = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_MCLK1, mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

#endif

hw_params_exit:
	return errorCode;
}

static struct snd_soc_ops i1evk_hifi_ops = {
	.hw_params = i1evk_hifi_hw_params,
};

static struct snd_soc_dai_link i1evk_dai_link = {
	.name = "WM8994",
	.stream_name = "WM8994-STREAM",
	.cpu_dai_name = "ambarella-i2s.0",
	.codec_dai_name = "wm8994-aif1",
	.platform_name = "ambarella-pcm-audio",
	.codec_name = "wm8994-codec",
	.init = i1evk_wm8994_init,
	.ops = &i1evk_hifi_ops,
};

static struct snd_soc_card snd_soc_card_i1evk = {
	.name = "I1EVK",
	.dai_link = &i1evk_dai_link,
	.num_links = 1,
};


static struct platform_device *i1evk_snd_device;

static int __init i1evk_board_init(void)
{
	int errorCode = 0;

	i1evk_snd_device = platform_device_alloc("soc-audio", -1);
	if (!i1evk_snd_device)
		return -ENOMEM;

	platform_set_drvdata(i1evk_snd_device, &snd_soc_card_i1evk);

	errorCode = platform_device_add(i1evk_snd_device);
	if (errorCode)
		goto i1sevk_board_init_exit;

	return 0;

i1sevk_board_init_exit:
	platform_device_put(i1evk_snd_device);
	return errorCode;
}

static void __exit i1evk_board_exit(void)
{
	if (gpio_jack) {
		snd_soc_jack_free_gpios(&hs_jack,
			ARRAY_SIZE(hs_jack_gpios), hs_jack_gpios);
		snd_soc_jack_free_gpios(&av_jack,
			ARRAY_SIZE(av_jack_gpios), av_jack_gpios);
	} else {
		platform_device_unregister(&headset_switch_device);
	}

	platform_device_unregister(i1evk_snd_device);
}

module_init(i1evk_board_init);
module_exit(i1evk_board_exit);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Amabrella i1sevk Board with WM8994 Codec for ALSA");
MODULE_LICENSE("GPL");
