/*
 * ak4642.c  --  AK4642 ALSA Soc Audio driver
 *
 * Copyright 2009 Ambarella Ltd.
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Based on ak4535.c by Richard Purdie
 *
 * History:
 *	2009/05/29 - [Cao Rongrong] Created file
 *	2010/10/25 - [Cao Rongrong] Port to 2.6.36+
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/ak4642_amb.h>

#include "ak4642_amb.h"

#define AK4642_VERSION "0.2"


/* codec private data */
struct ak4642_priv {
	unsigned int sysclk;
	void *control_data;
};

/*
 * ak4642 register cache
 */
static const u8 ak4642_reg[AK4642_CACHEREGNUM] = {
	0x00, 0x00, 0x01, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0xe1, 0xe1, 0x18, 0x00,
	0xe1, 0x18, 0x11, 0x08,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/*
 * read ak4642 register cache
 */
static inline unsigned int ak4642_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u8 *cache = codec->reg_cache;

	if (reg >= AK4642_CACHEREGNUM)
		return -1;

	return cache[reg];
}

/*
 * write to the AK4642 register space
 */
static int ak4642_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 *cache = codec->reg_cache;
	struct i2c_client *client = codec->control_data;

	if (reg >= AK4642_CACHEREGNUM)
		return -EIO;

	if (i2c_smbus_write_byte_data(client, reg, value)) {
		pr_err("AK4642: I2C write failed\n");
		return -EIO;
	}
	/* We've written to the hardware, so update the cache */
	cache[reg] = value;

	return 0;
}

static int ak4642_sync(struct snd_soc_codec *codec)
{
	u8 *cache = codec->reg_cache;
	int i, r = 0;

	for (i = 0; i < AK4642_CACHEREGNUM; i++)
		r |= snd_soc_write(codec, i, cache[i]);

	return r;
};


/****************   ALSA Controls and widgets   **************/

static int ak4642_get_mic_gain(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned short val, val1, val2;

	val1 = snd_soc_read(codec, AK4642_SIG1);
	val2 = snd_soc_read(codec, AK4642_SIG2);
	val = (val2 & 0x20) >> 4 | (val1 & 0x01);

	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int ak4642_set_mic_gain(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned short val, val1, val2;

	val1 = snd_soc_read(codec, AK4642_SIG1);
	val2 = snd_soc_read(codec, AK4642_SIG2);
	val = (val2 & 0x20) >> 4 | (val1 & 0x01);

	if (val == ucontrol->value.integer.value[0])
		return 0;

	val = ucontrol->value.integer.value[0];
	val1 &= 0xfe;
	val1 |= (val & 0x01);
	snd_soc_write(codec, AK4642_SIG1, val1);
	val2 &= 0xdf;
	val2 |= ((val & 0x02) << 4);
	snd_soc_write(codec, AK4642_SIG2, val2);

	return 1;
}

static int ak4642_get_alc_gain(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned short val, val1, val2;

	val1 = snd_soc_read(codec, AK4642_ALC1);
	val2 = snd_soc_read(codec, AK4642_ALC3);
	val = (val2 & 0x80) >> 6 | (val1 & 0x02) >> 1;

	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int ak4642_set_alc_gain(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned short val, val1, val2;

	val1 = snd_soc_read(codec, AK4642_ALC1);
	val2 = snd_soc_read(codec, AK4642_ALC3);
	val = (val2 & 0x80) >> 6 | (val1 & 0x02) >> 1;

	if (val == ucontrol->value.integer.value[0])
		return 0;

	val = ucontrol->value.integer.value[0];
	val1 &= 0xfd;
	val1 |= ((val & 0x01) << 1);
	snd_soc_write(codec, AK4642_ALC1, val1);
	val2 &= 0x7f;
	val2 |= ((val & 0x02) << 6);
	snd_soc_write(codec, AK4642_ALC3, val2);

	return 1;
}

static int ak4642_get_input_mux(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned short val, val1, val2;

	val1 = snd_soc_read(codec, AK4642_PM3);
	val2 = snd_soc_read(codec, AK4642_SIG1);

	if(((val1 & 0x1e) == 0x06) && ((val2 & 0x04) == 0x00))
		val = AK4642_LINE_IN_ON;
	else if(((val1 & 0x18) == 0x18) && ((val2 & 0x04) == 0x04))
		val = AK4642_BOTH_MIC_ON;
	else
		val = AK4642_INPUT_UNKNOWN;

	ucontrol->value.integer.value[0] = val;
	return 0;
}

static int ak4642_set_input_mux(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned short val, val1, val2;

	val1 = snd_soc_read(codec, AK4642_PM3);
	val2 = snd_soc_read(codec, AK4642_SIG1);

	if(((val1 & 0x1e) == 0x06) && ((val2 & 0x04) == 0x00))
		val = AK4642_LINE_IN_ON;
	else if(((val1 & 0x18) == 0x18) && ((val2 & 0x04) == 0x04))
		val = AK4642_BOTH_MIC_ON;
	else
		val = AK4642_INPUT_UNKNOWN;

	if (val == ucontrol->value.integer.value[0])
		return 0;

	val = ucontrol->value.integer.value[0];

	switch(val){
	case AK4642_LINE_IN_ON:
		snd_soc_update_bits(codec, AK4642_PM3, 0x18, 0x06);
		snd_soc_update_bits(codec, AK4642_SIG1, 0x05, 0);
		snd_soc_update_bits(codec, AK4642_SIG2, 0x20, 0);
		break;
	case AK4642_BOTH_MIC_ON:
		snd_soc_update_bits(codec, AK4642_PM3, 0x18, 0x18);
		snd_soc_update_bits(codec, AK4642_SIG1, 0x05, 0x05);
		snd_soc_update_bits(codec, AK4642_SIG2, 0x20, 0x20);
		break;
	case AK4642_INPUT_UNKNOWN:
		return 0;
	}

	return 1;
}


static const char *ak4642_lo_gain[] = {"+0db", "+2db"};
static const char *ak4642_hp_gain[] = {"+0db", "+3.6db"};
static const char *ak4642_sp_gain[] = {"+4.43db", "+6.43db", "+10.65db", "+12.65db"};
static const char *ak4642_vol_ctrl[] = {"Independent", "Dependent"};
static const char *ak4642_deemp[] = {"44.1kHz", "Off", "48kHz", "32kHz"};
static const char *ak4642_hp_out[] = {"Stereo", "Mono"};
static const char *ak4642_lsrc[] = {"LIN1", "LIN2"};
static const char *ak4642_rsrc[] = {"RIN1", "RIN2"};
static const char *ak4642_eq_gain[] = {"+0db", "+12db", "+24db"};
static const char *ak4642_fil_sel[] = {"HPF", "LPF"};
static const char *ak4642_mic_gain[] = {"0db", "+20db", "+26db", "+32db"};
static const char *ak4642_alc_gain[] = {"0.375db", "0.750db", "1.125db", "1.500db"};
static const char *ak4642_input_mux[] = {"Line-in", "Both Mic", "Unknown"};

static const struct soc_enum ak4642_enum[] = {
	SOC_ENUM_SINGLE(AK4642_SIG2, 7, 2, ak4642_lo_gain),
	SOC_ENUM_SINGLE(AK4642_SIG2, 3, 4, ak4642_sp_gain),
	SOC_ENUM_SINGLE(AK4642_MODE3, 4, 2, ak4642_vol_ctrl),
	SOC_ENUM_SINGLE(AK4642_MODE3, 0, 4, ak4642_deemp),
	SOC_ENUM_SINGLE(AK4642_MODE4, 3, 2, ak4642_vol_ctrl),
	SOC_ENUM_SINGLE(AK4642_MODE4, 2, 2, ak4642_hp_out),
	SOC_ENUM_SINGLE(AK4642_PM3, 5, 2, ak4642_hp_gain),
	SOC_ENUM_SINGLE(AK4642_PM3, 1, 2, ak4642_lsrc),
	SOC_ENUM_SINGLE(AK4642_PM3, 2, 2, ak4642_rsrc),
	SOC_ENUM_SINGLE(AK4642_FSEL, 6, 3, ak4642_eq_gain),
	SOC_ENUM_SINGLE(AK4642_F3EF1, 7, 2, ak4642_fil_sel),
	SOC_ENUM_SINGLE(AK4642_E1EF1, 7, 2, ak4642_fil_sel),
	SOC_ENUM_SINGLE_EXT(4, ak4642_mic_gain),
	SOC_ENUM_SINGLE_EXT(4, ak4642_alc_gain),
	SOC_ENUM_SINGLE_EXT(3, ak4642_input_mux),
};

static const struct snd_kcontrol_new ak4642_snd_controls[] = {
	SOC_SINGLE("HP Mute Switch", AK4642_PM2, 6, 1, 0),
	SOC_ENUM("Line Out Gain", ak4642_enum[0]),
	SOC_ENUM("Speaker Gain", ak4642_enum[1]),
	SOC_ENUM("Headphone Output Mode", ak4642_enum[5]),
	SOC_ENUM("Headphone Gain", ak4642_enum[6]),
	SOC_SINGLE("Mic Mute Switch", AK4642_SIG1, 2, 1, 1),

	SOC_SINGLE("ALC Switch", AK4642_ALC1, 5, 1, 0),
	SOC_SINGLE("ALC ZC Time", AK4642_TIMER, 4, 3, 0),
	SOC_SINGLE("ALC Recovery Time", AK4642_TIMER, 2, 3, 0),
	SOC_SINGLE("ALC ZC Detection Switch", AK4642_ALC1, 4, 1, 1),
	SOC_SINGLE("ALC ATT Step", AK4642_TIMER, 2, 3, 0),
	SOC_SINGLE("ALC Volume", AK4642_ALC2, 0, 255, 0),

	SOC_SINGLE("Left Capture Volume", AK4642_LIVOL, 0, 255, 0),
	SOC_SINGLE("Right Capture Volume", AK4642_RIVOL, 0, 255, 0),
	SOC_SINGLE("Left Playback Volume", AK4642_LDVOL, 0, 255, 1),
	SOC_SINGLE("Right Playback Volume", AK4642_RDVOL, 0, 255, 1),
	SOC_ENUM("Playback Volume Control Mode", ak4642_enum[2]),
	SOC_ENUM("Capture Volume Control Mode", ak4642_enum[4]),
	SOC_SINGLE("Bass Boost Volume", AK4642_MODE3, 2, 3, 0),
	SOC_ENUM("Playback Deemphasis", ak4642_enum[3]),

	SOC_SINGLE("Left Differential Swtich", AK4642_PM3, 3, 1, 0),
	SOC_SINGLE("Right Differential Swtich", AK4642_PM3, 4, 1, 0),
	/* ADC Source Selector is only available when differential switch is off */
	SOC_ENUM("Left ADC Source", ak4642_enum[7]),
	SOC_ENUM("Right ADC Source", ak4642_enum[8]),
	SOC_ENUM_EXT("Input Mux", ak4642_enum[14],
		ak4642_get_input_mux, ak4642_set_input_mux),

	SOC_ENUM("EQ Gain Select", ak4642_enum[9]),
	SOC_SINGLE("Emphasis Filter Switch", AK4642_FSEL, 2, 1, 0),
	SOC_ENUM("Emphasis Filter Select", ak4642_enum[10]),
	SOC_SINGLE("Gain Compensation Filter Switch", AK4642_FSEL, 3, 1, 0),
	SOC_SINGLE("Wind-noise Filter Switch", AK4642_FSEL, 4, 1, 0),
	SOC_ENUM("Wind-noise Filter Select", ak4642_enum[11]),
	SOC_SINGLE("Emphasis Filter 1 low Coeff", AK4642_F3EF0, 0, 255, 0),
	SOC_SINGLE("Emphasis Filter 1 high Coeff", AK4642_F3EF1, 0, 63, 0),
	SOC_SINGLE("Emphasis Filter 2 low Coeff", AK4642_F3EF2, 0, 255, 0),
	SOC_SINGLE("Emphasis Filter 2 high Coeff", AK4642_F3EF3, 0, 63, 0),
	SOC_SINGLE("Gain Compensation Filter 1 low Coeff", AK4642_EQEF0, 0, 255, 0),
	SOC_SINGLE("Gain Compensation Filter 1 high Coeff", AK4642_EQEF1, 0, 255, 0),
	SOC_SINGLE("Gain Compensation Filter 2 low Coeff", AK4642_EQEF2, 0, 255, 0),
	SOC_SINGLE("Gain Compensation Filter 2 high Coeff", AK4642_EQEF3, 0, 63, 0),
	SOC_SINGLE("Gain Compensation Filter 3 low Coeff", AK4642_EQEF4, 0, 255, 0),
	SOC_SINGLE("Gain Compensation Filter 3 high Coeff", AK4642_EQEF5, 0, 255, 0),
	SOC_SINGLE("Wind-noise Filter 1 low Coeff", AK4642_F3EF0, 0, 255, 0),
	SOC_SINGLE("Wind-noise Filter 1 high Coeff", AK4642_F3EF1, 0, 63, 0),
	SOC_SINGLE("Wind-noise Filter 2 low Coeff", AK4642_F3EF2, 0, 255, 0),
	SOC_SINGLE("Wind-noise Filter 2 high Coeff", AK4642_F3EF3, 0, 63, 0),
	SOC_ENUM_EXT("Mic Gain", ak4642_enum[12],
		ak4642_get_mic_gain, ak4642_set_mic_gain),
	SOC_ENUM_EXT("ALC Recovery Gain", ak4642_enum[13],
		ak4642_get_alc_gain, ak4642_set_alc_gain),
};

/* Mono 1 Mixer */
static const struct snd_kcontrol_new ak4642_hp_mixer_controls[] = {
	SOC_DAPM_SINGLE("HP Playback Switch", AK4642_MODE4, 0, 1, 0),
	SOC_DAPM_SINGLE("MIN HP Switch", AK4642_MODE4, 1, 1, 0),
};

/* Stereo Line Out Mixer */
static const struct snd_kcontrol_new ak4642_lo_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line Playback Switch", AK4642_SIG1, 4, 1, 0),
	SOC_DAPM_SINGLE("MIN LO Switch", AK4642_SIG2, 2, 1, 0),
};

/* Input Mixer */
static const struct snd_kcontrol_new ak4642_sp_mixer_controls[] = {
	SOC_DAPM_SINGLE("SP Playback Switch", AK4642_SIG1, 5, 1, 0),
	SOC_DAPM_SINGLE("MIN SP Switch", AK4642_SIG1, 6, 1, 0),
};

/* line out switch */
static const struct snd_kcontrol_new ak4642_lo_control =
	SOC_DAPM_SINGLE("Switch", AK4642_PM1, 3, 1, 0);
/* speaker switch */
static const struct snd_kcontrol_new ak4642_sp_control =
	SOC_DAPM_SINGLE("Switch", AK4642_SIG1, 7, 1, 0);

/* ak4642 dapm widgets */
static const struct snd_soc_dapm_widget ak4642_dapm_widgets[] = {
	/* OUTPUT */
	SND_SOC_DAPM_MIXER("Line Out Mixer", SND_SOC_NOPM, 0, 0,
			&ak4642_lo_mixer_controls[0],
			ARRAY_SIZE(ak4642_lo_mixer_controls)),
	SND_SOC_DAPM_MIXER("Headphone Mixer", SND_SOC_NOPM, 0, 0,
			&ak4642_hp_mixer_controls[0],
			ARRAY_SIZE(ak4642_hp_mixer_controls)),
	SND_SOC_DAPM_MIXER("Speaker Mixer", SND_SOC_NOPM, 0, 0,
			&ak4642_sp_mixer_controls[0],
			ARRAY_SIZE(ak4642_sp_mixer_controls)),

	SND_SOC_DAPM_DAC("DAC", "Playback", AK4642_PM1, 2, 0),
	SND_SOC_DAPM_PGA("Spk Amp", AK4642_PM1, 4, 0, NULL, 0),
	SND_SOC_DAPM_PGA("HP L Amp", AK4642_PM2, 5, 0, NULL, 0),
	SND_SOC_DAPM_PGA("HP R Amp", AK4642_PM2, 4, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Line Out Pga", AK4642_SIG2, 6, 1, NULL, 0),
	SND_SOC_DAPM_SWITCH("Speaker Enable", SND_SOC_NOPM, 0, 0,
			&ak4642_sp_control),
	SND_SOC_DAPM_SWITCH("Line Out Enable", SND_SOC_NOPM, 0, 0,
			&ak4642_lo_control),
	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),
	SND_SOC_DAPM_OUTPUT("HPL"),
	SND_SOC_DAPM_OUTPUT("HPR"),
	SND_SOC_DAPM_OUTPUT("SPP"),
	SND_SOC_DAPM_OUTPUT("SPN"),

	/* INPUT */
	SND_SOC_DAPM_ADC("Left ADC", "Capture", AK4642_PM1, 0, 0),
	SND_SOC_DAPM_ADC("Right ADC", "Capture", AK4642_PM3, 0, 0),
	SND_SOC_DAPM_PGA("MIN Input", AK4642_PM1, 5, 0, NULL, 0),
	SND_SOC_DAPM_MICBIAS("Mic Bias", AK4642_SIG1, 2, 0),

	SND_SOC_DAPM_INPUT("LIN1"),
	SND_SOC_DAPM_INPUT("RIN1"),
	SND_SOC_DAPM_INPUT("LIN2"),
	SND_SOC_DAPM_INPUT("RIN2"),
	SND_SOC_DAPM_INPUT("MIN"),
};

static const struct snd_soc_dapm_route audio_map[] = {
	/*line out mixer */
	{"Line Out Mixer", "Line Playback Switch", "DAC"},
	{"Line Out Mixer", "MIN LO Switch", "MIN Input"},

	/* headphone mixer */
	{"Headphone Mixer", "HP Playback Switch", "DAC"},
	{"Headphone Mixer", "MIN HP Switch", "MIN Input"},

	/*speaker mixer */
	{"Speaker Mixer", "SP Playback Switch", "DAC"},
	{"Speaker Mixer", "MIN SP Switch", "MIN Input"},

	/* line out */
	{"Line Out Pga", NULL, "Line Out Mixer"},
	{"Line Out Enable", "Switch", "Line Out Pga"},
	{"LOUT", NULL, "Line Out Enable"},
	{"ROUT", NULL, "Line Out Enable"},

	/* left headphone */
	{"HP L Amp", NULL, "Headphone Mixer"},
	{"HPL", NULL, "HP L Amp"},

	/* right headphone */
	{"HP R Amp", NULL, "Headphone Mixer"},
	{"HPR", NULL, "HP R Amp"},

	/* speaker */
	{"Spk Amp", NULL, "Speaker Mixer"},
	{"Speaker Enable", "Switch", "Spk Amp"},
	{"SPP", NULL, "Speaker Enable"},
	{"SPN", NULL, "Speaker Enable"},

	/* INPUT */
	{"MIN Input", NULL, "MIN"},
	{"Left ADC", NULL, "LIN1"},
	{"Left ADC", NULL, "LIN2"},
	{"Right ADC", NULL, "RIN1"},
	{"Right ADC", NULL, "RIN2"},
};

static int ak4642_add_widgets(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_new_controls(dapm, ak4642_dapm_widgets,
				  ARRAY_SIZE(ak4642_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, audio_map, ARRAY_SIZE(audio_map));

	return 0;
}

static int ak4642_set_dai_sysclk(struct snd_soc_dai *codec_dai,
	int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ak4642_priv *ak4642 = snd_soc_codec_get_drvdata(codec);

	ak4642->sysclk = freq;
	return 0;
}

static int ak4642_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct ak4642_priv *ak4642 = snd_soc_codec_get_drvdata(codec);
	int rate = params_rate(params), fs = 256;
	u8 mode = snd_soc_read(codec, AK4642_MODE2) & 0xc0;

	if (rate)
		fs = ak4642->sysclk / rate;

	/* set fs */
	switch (fs) {
	case 1024:
		mode |= 0x1;
		break;
	case 512:
		mode |= 0x3;
		break;
	case 256:
		mode |= 0x0;
		break;
	}

	/* set rate */
	snd_soc_write(codec, AK4642_MODE2, mode);
	return 0;
}

static int ak4642_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 mode1 = 0, mode2 = 0;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		snd_soc_update_bits(codec, AK4642_PM2, 0x08, 0);
		break;
	default:
		return -EINVAL;
	}

	mode1 = snd_soc_read(codec, AK4642_MODE1);
	mode2 = snd_soc_read(codec, AK4642_MODE2);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		mode1 |= 0x3;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		mode1 &= ~0x3;
		mode2 &= ~0x18;
		mode2 |= 0x10;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		mode1 &= ~0x3;
		mode2 &= ~0x18;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_RIGHT_J:
		pr_info("We don't implement this format (%d) yet.\n", fmt);
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, AK4642_MODE1, mode1);
	snd_soc_write(codec, AK4642_MODE2, mode2);
	return 0;
}

static int ak4642_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	if (mute){
		snd_soc_update_bits(codec, AK4642_MODE3, 0x20, 0x20);
		snd_soc_update_bits(codec, AK4642_PM2, 0x40, 0);
	}
	else{
		snd_soc_update_bits(codec, AK4642_MODE3, 0x20, 0);
		snd_soc_update_bits(codec, AK4642_PM2, 0x40, 0x40);
	}

	return 0;
}

static int ak4642_set_bias_level(struct snd_soc_codec *codec,
	enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		/* Everything is ON */
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		snd_soc_update_bits(codec, AK4642_PM1, 0x40, 0x40);
		break;
	case SND_SOC_BIAS_OFF:
		/* Everything is OFF */
		snd_soc_update_bits(codec, AK4642_PM1, 0x40, 0);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}


#define AK4642_RATES		SNDRV_PCM_RATE_8000_48000
#define AK4642_FORMATS		SNDRV_PCM_FMTBIT_S16_LE

static struct snd_soc_dai_ops ak4642_dai_ops = {
	.hw_params =	ak4642_hw_params,
	.set_fmt =	ak4642_set_dai_fmt,
	.digital_mute =	ak4642_mute,
	.set_sysclk =	ak4642_set_dai_sysclk,
};

struct snd_soc_dai_driver ak4642_dai = {
	.name = "ak4642-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AK4642_RATES,
		.formats = AK4642_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AK4642_RATES,
		.formats = AK4642_FORMATS,},
	.ops = &ak4642_dai_ops,
};

static int ak4642_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	ak4642_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int ak4642_resume(struct snd_soc_codec *codec)
{
	ak4642_sync(codec);
	ak4642_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static int ak4642_probe(struct snd_soc_codec *codec)
{
	struct ak4642_priv *ak4642 = snd_soc_codec_get_drvdata(codec);
	struct ak4642_platform_data *ak4642_pdata;
	int ret;

	dev_info(codec->dev, "AK4642 Audio Codec %s", AK4642_VERSION);

	codec->control_data = ak4642->control_data;

	ak4642_pdata = codec->dev->platform_data;
	if (!ak4642_pdata)
		return -EINVAL;

	if (gpio_is_valid(ak4642_pdata->rst_pin)) {
		ret = gpio_request(ak4642_pdata->rst_pin, "ak4642 reset");
		if (ret < 0)
			return ret;
	} else {
		return -ENODEV;
	}

	/* Reset AK4642 codec */
	gpio_direction_output(ak4642_pdata->rst_pin, GPIO_LOW);
	msleep(ak4642_pdata->rst_delay);
	gpio_direction_output(ak4642_pdata->rst_pin, GPIO_HIGH);

	/* power on device */
	ak4642_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/* Initial some register */
	/* Select Input to ADC */
	snd_soc_update_bits(codec, AK4642_PM3, 0x06, 0x06);
	/* Open DAC to Line-Out */
	snd_soc_update_bits(codec, AK4642_SIG1, 0x10, 0x10);
	/* Open DAC to Headphone */
	snd_soc_update_bits(codec, AK4642_MODE4, 0x01, 0x01);
	/* Open Line-Out Switch */
	snd_soc_update_bits(codec, AK4642_PM1, 0x08, 0x08);
	snd_soc_write(codec, AK4642_LIVOL, 0x91);	/* Input 0db */
	snd_soc_write(codec, AK4642_RIVOL, 0x91);	/* Input 0db */
	/* Mic-Amp 0db */
	snd_soc_update_bits(codec, AK4642_SIG1, 0x01, 0);
	/* Mic-Amp 0db */
	snd_soc_update_bits(codec, AK4642_SIG2, 0x20, 0);

	snd_soc_add_controls(codec, ak4642_snd_controls,
				ARRAY_SIZE(ak4642_snd_controls));
	ak4642_add_widgets(codec);

	return 0;
}

static int ak4642_remove(struct snd_soc_codec *codec)
{
	struct ak4642_platform_data *ak4642_pdata;

	ak4642_set_bias_level(codec, SND_SOC_BIAS_OFF);

	ak4642_pdata = codec->dev->platform_data;
	gpio_free(ak4642_pdata->rst_pin);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_ak4642 = {
	.probe =	ak4642_probe,
	.remove =	ak4642_remove,
	.suspend =	ak4642_suspend,
	.resume =	ak4642_resume,
	.read =		ak4642_read_reg_cache,
	.write =	ak4642_write,
	.set_bias_level = ak4642_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(ak4642_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = ak4642_reg,
	.reg_cache_step = 1,
};

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
static int ak4642_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct ak4642_priv *ak4642;
	int ret;

	ak4642 = kzalloc(sizeof(struct ak4642_priv), GFP_KERNEL);
	if (ak4642 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, ak4642);
	ak4642->control_data = i2c;

	ret =  snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_ak4642, &ak4642_dai, 1);
	if (ret < 0)
		kfree(ak4642);

	return ret;
}

static int ak4642_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	kfree(i2c_get_clientdata(i2c));
	return 0;
}

static const struct i2c_device_id ak4642_i2c_id[] = {
	{ "ak4642", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak4642_i2c_id);

static struct i2c_driver ak4642_i2c_driver = {
	.driver = {
		.name = "ak4642-codec",
		.owner = THIS_MODULE,
	},
	.probe =    ak4642_i2c_probe,
	.remove =   ak4642_i2c_remove,
	.id_table = ak4642_i2c_id,
};

#endif

static int __init ak4642_modinit(void)
{
	int ret;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&ak4642_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register UDA1380 I2C driver: %d\n", ret);
#endif
	return 0;
}
module_init(ak4642_modinit);

static void __exit ak4642_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&ak4642_i2c_driver);
#endif
}
module_exit(ak4642_exit);

MODULE_DESCRIPTION("Soc AK4642 driver");
MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE("GPL");

