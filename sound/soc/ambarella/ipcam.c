/*
 * sound/soc/ipcam.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 * History:
 *	2008/xx/xx - [Anthony Ginger] Created file
 *	2009/03/05 - [Cao Rongrong] Correct and Add Controls,
 *				    Modify function and variable names
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
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
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <mach/hardware.h>
#include <plat/audio.h>

#include "../codecs/ambarella_auc.h"


/*****************************************************************************/

static unsigned int spk_gpio = 52;
module_param(spk_gpio, uint, S_IRUGO);
MODULE_PARM_DESC(spk_gpio, "The power on gpio of sound input, if you have.");

static unsigned int spk_level = GPIO_HIGH;
module_param(spk_level, uint, S_IRUGO);
MODULE_PARM_DESC(spk_level, "The gpio power on level, if you set power on gpio id.");

static unsigned int spk_delay = 1;
module_param(spk_delay, uint, S_IRUGO);
MODULE_PARM_DESC(spk_delay, "The gpio power on delay(ms), if you set power on gpio id.");


static unsigned int mic_gpio = 53;
module_param(mic_gpio, uint, S_IRUGO);
MODULE_PARM_DESC(mic_gpio, "The power on gpio of sound output, if you have.");

static unsigned int mic_level = GPIO_HIGH;
module_param(mic_level, uint, S_IRUGO);
MODULE_PARM_DESC(mic_level, "The gpio power on level, if you set power on gpio id.");

static unsigned int mic_delay = 1;
module_param(mic_delay, uint, S_IRUGO);
MODULE_PARM_DESC(mic_delay, "The gpio power on delay(ms), if you set power on gpio id.");

/*****************************************************************************/
#define AMBA_MIC_ON		0
#define AMBA_MIC_OFF		1
#define AMBA_SPK_ON		0
#define AMBA_SPK_OFF		1

/* mic and speaker function: default ON */
static int amba_mic_func = AMBA_MIC_ON;
static int amba_spk_func = AMBA_SPK_ON;

static void ipcam_ext_control(struct snd_soc_codec *codec)
{
	int errorCode = 0;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	/* set up mic connection */
	if (amba_mic_func == AMBA_MIC_ON){
		errorCode = gpio_direction_output(mic_gpio, !!mic_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Mic-Ctrl GPIO high\n");
			goto err_exit;
		}
		mdelay(mic_delay);
		snd_soc_dapm_enable_pin(dapm, "Mic Jack");
	}else{
		errorCode = gpio_direction_output(mic_gpio, !mic_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Mic-Ctrl GPIO low\n");
			goto err_exit;
		}
		mdelay(mic_delay);
		snd_soc_dapm_disable_pin(dapm, "Mic Jack");
	}

	if (amba_spk_func == AMBA_SPK_ON){
		errorCode = gpio_direction_output(spk_gpio, !!spk_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Spk-Ctrl GPIO high\n");
			goto err_exit;
		}
		mdelay(spk_delay);
		snd_soc_dapm_enable_pin(dapm, "Ext Spk");
	}else{
		errorCode = gpio_direction_output(spk_gpio, !spk_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Spk-Ctrl GPIO low\n");
			goto err_exit;
		}
		mdelay(spk_delay);
		snd_soc_dapm_disable_pin(dapm, "Ext Spk");
	}

	/* signal a DAPM event */
	snd_soc_dapm_sync(dapm);

err_exit:
	return;
}


static int ipcam_board_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;

	ipcam_ext_control(codec);

	return 0;
}

static int ipcam_board_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int errorCode = 0, mclk, oversample;

	switch (params_rate(params)) {
	case 8000:
		mclk = AudioCodec_4_096M;
		oversample = AudioCodec_512xfs;
		break;
	case 11025:
		mclk = AudioCodec_5_6448M;
		oversample = AudioCodec_512xfs;
		break;
	case 16000:
		mclk = AudioCodec_4_096M;
		oversample = AudioCodec_256xfs;
		break;
	case 22050:
		mclk = AudioCodec_5_6448M;
		oversample = AudioCodec_256xfs;
		break;
	case 32000:
		mclk = AudioCodec_8_192M;
		oversample = AudioCodec_256xfs;
		break;
	case 44100:
		mclk = AudioCodec_11_2896M;
		oversample = AudioCodec_256xfs;
		break;
	case 48000:
		mclk = AudioCodec_12_288M;
		oversample = AudioCodec_256xfs;
		break;
	default:
		errorCode = -EINVAL;
		goto hw_params_exit;
	}

	/* set the I2S system data format*/
	errorCode = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		printk(KERN_ERR "can't set codec DAI configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		printk(KERN_ERR "can't set cpu DAI configuration\n");
		goto hw_params_exit;
	}

	/* set the I2S system clock*/
	errorCode = snd_soc_dai_set_sysclk(cpu_dai, AMBARELLA_CLKSRC_ONCHIP, mclk, 0);
	if (errorCode < 0) {
		printk(KERN_ERR "can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_clkdiv(codec_dai, A2AUC_CLKDIV_LRCLK, oversample);
	if (errorCode < 0) {
		printk(KERN_ERR "can't set codec MCLK/SF ratio\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_clkdiv(cpu_dai, AMBARELLA_CLKDIV_LRCLK, oversample);
	if (errorCode < 0) {
		printk(KERN_ERR "can't set cpu MCLK/SF ratio\n");
		goto hw_params_exit;
	}

hw_params_exit:
	return errorCode;
}


static struct snd_soc_ops ipcam_board_ops = {
	.startup = ipcam_board_startup,
	.hw_params = ipcam_board_hw_params,
};

static int ipcam_get_mic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = amba_mic_func;
	return 0;
}

static int ipcam_set_mic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (amba_mic_func == ucontrol->value.integer.value[0])
		return 0;

	amba_mic_func = ucontrol->value.integer.value[0];
	ipcam_ext_control(codec);
	return 1;
}

static int ipcam_get_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = amba_spk_func;
	return 0;
}

static int ipcam_set_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec =  snd_kcontrol_chip(kcontrol);

	if (amba_spk_func == ucontrol->value.integer.value[0])
		return 0;

	amba_spk_func = ucontrol->value.integer.value[0];
	ipcam_ext_control(codec);
	return 1;
}

static int ipcam_spk_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	int errorCode = 0;

	if (SND_SOC_DAPM_EVENT_ON(event)){
		errorCode = gpio_direction_output(spk_gpio, !!spk_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Spk-Ctrl GPIO high\n");
			goto err_exit;
		}
	} else {
		errorCode = gpio_direction_output(spk_gpio, !spk_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Spk-Ctrl GPIO low\n");
			goto err_exit;
		}
	}
	mdelay(spk_delay);

err_exit:
	return errorCode;
}

static int ipcam_mic_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	int errorCode = 0;

	if (SND_SOC_DAPM_EVENT_ON(event)){
		errorCode = gpio_direction_output(mic_gpio, !!mic_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Mic-Ctrl GPIO high\n");
			goto err_exit;
		}
	} else {
		errorCode = gpio_direction_output(mic_gpio, !mic_level);
		if (errorCode < 0) {
			printk(KERN_ERR "Could not Set Mic-Ctrl GPIO low\n");
			goto err_exit;
		}
	}
	mdelay(mic_delay);

err_exit:
	return errorCode;
}

/* IPcam machine dapm widgets */
static const struct snd_soc_dapm_widget ipcam_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic Jack", ipcam_mic_event),
	SND_SOC_DAPM_SPK("Ext Spk", ipcam_spk_event),
	SND_SOC_DAPM_LINE("Line In", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),
};

/* IPcam machine audio map (connections to the a2auc pins) */
static const struct snd_soc_dapm_route ipcam_audio_map[] = {
	/* speaker is connected to SPOUT */
	{"Ext Spk", NULL, "SPOUT"},

	/* mic is connected to LLIN, RLIN*/
	{"LLIN", NULL, "Mic Jack"},
	{"RLIN", NULL, "Mic Jack"},

	/* Line Out is connected to LLOUT, RLOUT */
	{"Line Out", NULL, "LLOUT"},
	{"Line Out", NULL, "RLOUT"},

	/* Line In is connected to LLIN, RLIN */
	{"LLIN", NULL, "Line In"},
	{"RLIN", NULL, "Line In"},
};

static const char *mic_function[] = {"On", "Off"};
static const char *spk_function[] = {"On", "Off"};
static const struct soc_enum ipcam_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_function), mic_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_function), spk_function),
};

static const struct snd_kcontrol_new a2auc_ipcam_controls[] = {
	SOC_ENUM_EXT("Mic Function", ipcam_enum[0], ipcam_get_mic, ipcam_set_mic),
	SOC_ENUM_EXT("Spk Function", ipcam_enum[1], ipcam_get_spk, ipcam_set_spk),
};

static int ipcam_a2auc_init(struct snd_soc_pcm_runtime *rtd)
{
	int errorCode = 0;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_nc_pin(dapm, "LHPOUT");
	snd_soc_dapm_nc_pin(dapm, "RHPOUT");

	/* Add IPcam specific controls */
	snd_soc_add_controls(codec, a2auc_ipcam_controls,
				ARRAY_SIZE(a2auc_ipcam_controls));

	/* Add IPcam specific widgets */
	errorCode = snd_soc_dapm_new_controls(dapm,
		ipcam_dapm_widgets,
		ARRAY_SIZE(ipcam_dapm_widgets));
	if (errorCode) {
		goto init_exit;
	}

	/* Set up IPcam specific audio path ipcam_audio_map */
	errorCode = snd_soc_dapm_add_routes(dapm,
		ipcam_audio_map,
		ARRAY_SIZE(ipcam_audio_map));
	if (errorCode) {
		goto init_exit;
	}

	errorCode = snd_soc_dapm_sync(dapm);

init_exit:
	return errorCode;
}

static struct snd_soc_dai_link ipcam_dai_link = {
	.name = "A2AUC",
	.stream_name = "A2AUC-STREAM",
	.cpu_dai_name = "ambarella-i2s.0",
	.platform_name = "ambarella-pcm-audio",
	.codec_dai_name = "a2auc-hifi",
	.codec_name = "a2auc-codec",
	.init = ipcam_a2auc_init,
	.ops = &ipcam_board_ops,
};

static struct snd_soc_card snd_soc_card_ipcam = {
	.name = "A2IPcam",
	.dai_link = &ipcam_dai_link,
	.num_links = 1,
};

static struct platform_device *ipcam_snd_device;

static int __init ipcam_board_init(void)
{
	int errorCode = 0;

	errorCode = gpio_request(mic_gpio, "Mic-Ctrl");
	if (errorCode < 0) {
		printk(KERN_ERR "Could not get Mic-Ctrl GPIO %d\n", mic_gpio);
		goto ipcam_board_init_exit2;
	}

	errorCode = gpio_request(spk_gpio, "Spk-Ctrl");
	if (errorCode < 0) {
		printk(KERN_ERR "Could not get Spk-Ctrl GPIO %d\n", spk_gpio);
		goto ipcam_board_init_exit1;
	}

	ipcam_snd_device =
		platform_device_alloc("soc-audio", -1);
	if (!ipcam_snd_device) {
		errorCode = -ENOMEM;
		goto ipcam_board_init_exit0;
	}

	platform_set_drvdata(ipcam_snd_device, &snd_soc_card_ipcam);

	errorCode = platform_device_add(ipcam_snd_device);
	if (errorCode) {
		platform_device_put(ipcam_snd_device);
		goto ipcam_board_init_exit0;
	}

	return 0;

ipcam_board_init_exit0:
	gpio_free(mic_gpio);
ipcam_board_init_exit1:
	gpio_free(spk_gpio);
ipcam_board_init_exit2:
	return errorCode;
}

static void __exit ipcam_board_exit(void)
{
	platform_device_unregister(ipcam_snd_device);
	gpio_free(spk_gpio);
	gpio_free(mic_gpio);
}

module_init(ipcam_board_init);
module_exit(ipcam_board_exit);

MODULE_AUTHOR("Anthony Ginger <hfjiang@ambarella.com>");
MODULE_DESCRIPTION("Amabrella A2 Board with internal Codec for ALSA");
MODULE_LICENSE("GPL");
MODULE_ALIAS("snd-soc-a2bub");

