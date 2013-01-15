/*
 * sound/soc/codecs/ambarella_auc.c
 *
 * History:
 *	2008/03/03 - [Eric Lee] created file
 *	2008/03/27 - [Cao Rongrong] Fix the pga regsister setup bug
 *	2008/04/16 - [Eric Lee] Removed the compiling warning
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *	2008/03/05 - [Cao Rongrong] Added widgets and controls
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

#include "ambarella_auc.h"


a2aucctrl_reg_t a2auc_reg;
static u32 a2auc_pga_gain_table[Gain_Table_Size];
static void a2auc_volume_control(u8 volume);

int a2auc_snd_soc_info_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 90;
	return 0;
}

int a2auc_snd_soc_put_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	unsigned char val, change = 0;

	val = Gain_Table_Size - 1 - ucontrol->value.integer.value[0];

	if (val >= Gain_Table_Size)
		val = Gain_Table_Size - 1;

	if(a2auc_pga_gain_table[val] != a2auc_reg.reg_10){
		change = 1;
		a2auc_volume_control(val);
	}

	return change;
}

int a2auc_snd_soc_get_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int i;

	for(i = 0; i < Gain_Table_Size; i++){
		if(a2auc_reg.reg_10 == a2auc_pga_gain_table[i])
			break;
	}

	ucontrol->value.integer.value[0] = Gain_Table_Size - 1 - i;

	return 0;
}

#define A2AUC_SOC_SINGLE(xname) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = a2auc_snd_soc_info_volsw, .get = a2auc_snd_soc_get_volsw,\
	.put = a2auc_snd_soc_put_volsw \
}

static const struct snd_kcontrol_new a2auc_snd_controls[] = {
	SOC_SINGLE("Capture Volume", 0x38, 0, 31, 0),
	SOC_SINGLE("ADC Capture Switch", 0x34, 2, 1, 1),
	A2AUC_SOC_SINGLE("Speaker Playback Volume"),
};

static const struct snd_soc_dapm_widget a2auc_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "Playback", 0x3c, 6, 1),
	SND_SOC_DAPM_OUTPUT("LLOUT"),
	SND_SOC_DAPM_OUTPUT("RLOUT"),
	SND_SOC_DAPM_OUTPUT("LHPOUT"),
	SND_SOC_DAPM_OUTPUT("RHPOUT"),
	SND_SOC_DAPM_OUTPUT("SPOUT"),

	SND_SOC_DAPM_ADC("ADC", "Capture", 0x34, 0, 1),
	SND_SOC_DAPM_INPUT("LLIN"),
	SND_SOC_DAPM_INPUT("RLIN"),
};

static const struct snd_soc_dapm_route intercon[] = {
	/* outputs */
	{"LLOUT", NULL, "DAC"},
	{"RLOUT", NULL, "DAC"},
	{"LHPOUT", NULL, "DAC"},
	{"RHPOUT", NULL, "DAC"},
	{"SPOUT", NULL, "DAC"},

	/* inputs */
	{"ADC", NULL, "LLIN"},
	{"ADC", NULL, "RLIN"},
};

static int a2auc_add_widgets(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_new_controls(dapm, a2auc_dapm_widgets,
				  ARRAY_SIZE(a2auc_dapm_widgets));
	/* set up audio path interconnects */
	snd_soc_dapm_add_routes(dapm, intercon, ARRAY_SIZE(intercon));

	return 0;
}

static u32 a2auc_pga_gain_table[Gain_Table_Size] = {
	/* 6dB */
	(AUC_DAC_GAIN_PGA1_6|AUC_DAC_GAIN_PGA2_0),
	/* 5dB ~ 0dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_0),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_0),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_0),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_0),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_0),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_0),
	/* -1dB ~ -6dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_1),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_1),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_1),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_1),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_1),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_1),
	/* -7dB ~ -12dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_2),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_2),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_2),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_2),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_2),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_2),
	/* -13dB ~ -18dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_3),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_3),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_3),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_3),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_3),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_3),
	/* -19dB ~ -24dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_4),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_4),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_4),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_4),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_4),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_4),
	/* -25dB ~ -30dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_5),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_5),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_5),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_5),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_5),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_5),
	/* -31dB ~ -36dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_6),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_6),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_6),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_6),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_6),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_6),
	/* -37dB ~ -42dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_7),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_7),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_7),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_7),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_7),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_7),
	/* -43dB ~ -48dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_8),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_8),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_8),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_8),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_8),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_8),
	/* -49dB ~ -54dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_9),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_9),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_9),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_9),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_9),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_9),
	/* -55dB ~ -60dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_a),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_a),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_a),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_a),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_a),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_a),
	/* -61dB ~ -66dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_b),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_b),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_b),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_b),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_b),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_b),
	/* -67dB ~ -72dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_c),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_c),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_c),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_c),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_c),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_c),
	/* -73dB ~ -78dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_d),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_d),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_d),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_d),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_d),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_d),
	/* -79dB ~ -84dB */
	(AUC_DAC_GAIN_PGA1_5|AUC_DAC_GAIN_PGA2_e),(AUC_DAC_GAIN_PGA1_4|AUC_DAC_GAIN_PGA2_e),
	(AUC_DAC_GAIN_PGA1_3|AUC_DAC_GAIN_PGA2_e),(AUC_DAC_GAIN_PGA1_2|AUC_DAC_GAIN_PGA2_e),
	(AUC_DAC_GAIN_PGA1_1|AUC_DAC_GAIN_PGA2_e),(AUC_DAC_GAIN_PGA1_0|AUC_DAC_GAIN_PGA2_e)
};

#if 0
static u32 a2auc_adc_cic_mult_table[ADC_CIC_MULT_LEVEL+1] =
{
		0x00000000, 0x00002000, 0x00004000, 0x00006000, 0x00008000, 0x0000a000, 0x0000c000, 0x0000e000,
		0x00010000, 0x00012000, 0x00014000, 0x00016000, 0x00018000, 0x0001a000, 0x0001c000, 0x0001e000,
		0x00020000, 0x00022000, 0x00024000, 0x00026000, 0x00028000, 0x0002a000, 0x0002c000, 0x0002e000,
		0x00030000, 0x00032000, 0x00034000, 0x00036000, 0x00038000, 0x0003a000, 0x0003c000, 0x0003e000,
		0x00040000, 0x00042000, 0x00044000, 0x00046000, 0x00048000, 0x0004a000, 0x0004c000, 0x0004e000,
		0x00050000, 0x00052000, 0x00054000, 0x00056000, 0x00058000, 0x0005a000, 0x0005c000, 0x0005e000,
		0x00060000, 0x00062000, 0x00064000, 0x00066000, 0x00068000, 0x0006a000, 0x0006c000, 0x0006e000,
		0x00070000, 0x00072000, 0x00074000, 0x00076000, 0x00078000, 0x0007a000, 0x0007c000, 0x0007e000,
		0x00080000
};
#endif

static u32 a2auc_droopcompensationfilter_table[DroopCompensationFilter_Size] =
{
	0x000FF48B, 0x00003491, 0x000FD8CB, 0x000F2780, 0x000232A9, 0x00054811
};

static u32 a2auc_imagesuppressionfilter_table[ImageSuppressionFilter_Size] =
{
	0x00000225, 0x00000981, 0x00001B3B, 0x00003C53, 0x00006D24,
	0x0000A94E, 0x0000E4D1, 0x000110D5, 0x00012165
};

static u32 a2auc_periodicfilter_table[PeriodicFilter_Size] =
{
	0x000FFFE4, 0x000FFE04, 0x000FFF85, 0x000004C5, 0x00000262,
	0x000FF5B8, 0x000FF9D3, 0x000012F6, 0x00000D8E, 0x000FDFDB,
	0x000FE59E, 0x00003310, 0x0000302B, 0x000FB207, 0x000FAA5B,
	0x00007501, 0x00009C96, 0x000F4E62, 0x000EB810, 0x00010D9E,
	0x00043FF0
};

#ifdef AUC_USE_DC_REMOVAL_FILTER
static u32 a2auc_dcremovalfilter_table[DCRemovalFilter_Size] =
{
	/* Stage 1 */
	0x00000000, 0x00000040, 0x000007f1, 0x000003c0, 0x00000000,
	0x00000000, 0x03c0d13b, 0x000003c0, 0x00000000, 0x00000000,
	/* Stage 2 */
	0x00000000, 0x00000040, 0x0000142e, 0x00000380, 0x000007f0,
	0x00000040, 0x0346d3bc, 0x00000380, 0xfcb9798f, 0x0000003f
};
#endif

static u32 a2auc_windnoisefilter_table[WindNoiseFilter_Size] =
{
	/* Stage 1 */
	0x00000000, 0x00000040, 0x00000010, 0x000003c0, 0x00000000,
	0x00000000, 0x4ae797e1, 0x000003c0, 0x00000000, 0x00000000,
	/* Stage 2 */
	0x00000000, 0x00000040, 0x002beeea, 0x00000380, 0x0000000f,
	0x00000040, 0x41daabd4, 0x00000380, 0xbe9ddf98, 0x0000003f
};

static u32 a2auc_deemphasisfilter_table[DeEmphasisFilter_Size] =
{
	/* Stage 1 */
	0x00000000, 0x00000040, 0xdd7e6059, 0x000003f1, 0x6ecab8f7, 0x000003d7
};

/* codec private data */
//struct wm8731_priv {
//	unsigned int sysclk;
//};

a2aucctrl_reg_t a2auc_reg={
	AUC_ENABLE_REG_VAL,
	AUC_DP_RESET_REG_VAL,
	AUC_OSR_REG_VAL,
	AUC_CONTROL_REG_VAL,
	AUC_STATUS_REG_VAL,
	AUC_CIC_REG_VAL,
	AUC_I2S_REG_VAL,
	0x00000000,	//Clock Latch, Unused
	AUC_DSM_GAIN_REG_VAL,
	AUC_EMA_REG_VAL,
	0x00000000,	//Unused
	0x00000000,	//Unused
	0x00000000,	//BG Control, Unused
	AUC_ADC_CONTROL_REG_VAL,
	AUC_ADC_VGA_GAIN_REG_VAL,
	AUC_DAC_CONTROL_REG_VAL,
	AUC_DAC_GAIN_REG_VAL
};

static u32 a2auc_check_state(void)
{
	u32 retval;
	retval = a2auc_read(AUC_ENABLE_REG);
	if(retval==(u32)0xBEEFBABE)
		return (AUC_RESET_STATE);
	else if(retval==(u32)0x00000000)
		return (AUC_IDLE_STATE);
	else
		return (AUC_NORMAL_STATE);
}

static void a2auc_set_droopcompensationfilter(void)
{
	int i;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		for(i=0 ; i<DroopCompensationFilter_Size ; i++)
		{
		a2auc_write(AUC_COEFF_START_DC_REG+(i<<2), a2auc_droopcompensationfilter_table[i]);
		a2auc_read(AUC_COEFF_START_DC_REG+(i<<2));
		}
	}
}

static void a2auc_set_imagesuppressionfilter(void)
{
	int i;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		for(i=0 ; i<ImageSuppressionFilter_Size ; i++)
		{
		a2auc_write(AUC_COEFF_START_IS_REG+(i<<2), a2auc_imagesuppressionfilter_table[i]);
		a2auc_read(AUC_COEFF_START_IS_REG+(i<<2));
		}
	}
}

void a2auc_set_periodicfilter(void)
{
	int i;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		for(i=0 ; i<PeriodicFilter_Size ; i++)
		{
		a2auc_write(AUC_COEFF_START_PD_REG+(i<<2), a2auc_periodicfilter_table[i]);
		a2auc_read(AUC_COEFF_START_PD_REG+(i<<2));
		}
	}
}

#ifdef AUC_USE_DC_REMOVAL_FILTER
static void a2auc_set_dcremovalfilter(void)
{
	int i;
	if (a2auc_check_state() != AUC_RESET_STATE) {
		for(i=0 ; i<DCRemovalFilter_Size ; i++) {
			a2auc_write(AUC_COEFF_START_HP_REG+(i<<2), a2auc_dcremovalfilter_table[i]);
			a2auc_read(AUC_COEFF_START_HP_REG+(i<<2));
		}
	}
}
#endif

static void a2auc_set_windnoisefilter(void)
{
	int i;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		for(i=0 ; i<WindNoiseFilter_Size ; i++)
		{
		#if 1
		a2auc_write(AUC_COEFF_START_WN_REG+(i<<2), a2auc_windnoisefilter_table[i]);
		#else
		a2auc_write(AUC_COEFF_START_WN_REG+(i<<2), a2auc_windnoisefilter_table[19][i]);
		#endif
		a2auc_read(AUC_COEFF_START_WN_REG+(i<<2));
		}
	}
}

static void a2auc_set_deemphasisfilter(void)
{
	int i;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		for(i=0 ; i<DeEmphasisFilter_Size ; i++)
		{
		a2auc_write(AUC_COEFF_START_DE_REG+(i<<2), a2auc_deemphasisfilter_table[i]);
		a2auc_read(AUC_COEFF_START_DE_REG+(i<<2));
		}
	}
}


static void a2auc_init_fiter(void)
{
	a2auc_set_droopcompensationfilter();
	a2auc_set_imagesuppressionfilter();
	a2auc_set_periodicfilter();
#ifdef AUC_USE_DC_REMOVAL_FILTER
	a2auc_set_dcremovalfilter();
#else
	a2auc_set_windnoisefilter();
#endif
	a2auc_set_deemphasisfilter();
}

static void a2auc_adc_on(void)
{
	//a2auc_reg.reg_00 = a2auc_read(AUC_ENABLE_REG);
	a2auc_reg.reg_00 = a2auc_reg.reg_00|AUC_ADC_ENABLE;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ENABLE_REG, a2auc_reg.reg_00);
	a2auc_read(AUC_ENABLE_REG);
	}
}

static void a2auc_adc_off(void)
{
	//a2auc_reg.reg_00 = a2auc_read(AUC_ENABLE_REG);
	a2auc_reg.reg_00 = a2auc_reg.reg_00&(~((u32)AUC_ADC_ENABLE));
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ENABLE_REG, a2auc_reg.reg_00);
	a2auc_read(AUC_ENABLE_REG);
	}
}

static void a2auc_adc_power_on(void)
{
	//a2auc_reg.reg_0d = (u32) a2auc_read(AUC_ADC_CONTROL_REG);
	a2auc_reg.reg_0d = a2auc_reg.reg_0d & (~((u32)AUC_ADC_CONTROL_ADC_PD)) ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ADC_CONTROL_REG, a2auc_reg.reg_0d);
	a2auc_read(AUC_ADC_CONTROL_REG);
	}
}

static void a2auc_adc_power_down(void)
{
	//a2auc_reg.reg_0d = (u32) a2auc_read(AUC_ADC_CONTROL_REG);
	a2auc_reg.reg_0d = a2auc_reg.reg_0d |AUC_ADC_CONTROL_ADC_PD;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ADC_CONTROL_REG, a2auc_reg.reg_0d);
	a2auc_read(AUC_ADC_CONTROL_REG);
	}
}

static void a2auc_adc_mute_on(void)
{
	//a2auc_reg.reg_0d = (u32) a2auc_read(AUC_ADC_CONTROL_REG);
	a2auc_reg.reg_0d = a2auc_reg.reg_0d |AUC_ADC_CONTROL_ADC_MUTE;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ADC_CONTROL_REG, a2auc_reg.reg_0d);
	a2auc_read(AUC_ADC_CONTROL_REG);
	}
}

static void a2auc_adc_mute_off(void)
{
	//a2auc_reg.reg_0d = (u32) a2auc_read(AUC_ADC_CONTROL_REG);
	a2auc_reg.reg_0d = a2auc_reg.reg_0d & (~((u32)AUC_ADC_CONTROL_ADC_MUTE)) ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ADC_CONTROL_REG, a2auc_reg.reg_0d);
	a2auc_read(AUC_ADC_CONTROL_REG);
	}
}

#if 0
static void a2auc_adc_dp_reset(void)
{
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_DP_RESET_REG, AUC_DP_RESET_ADC);
	a2auc_read(AUC_DP_RESET_REG);
	}
}
#endif

static void a2auc_dac_on(void)
{
	//a2auc_reg.reg_00 = a2auc_read(AUC_ENABLE_REG);
	a2auc_reg.reg_00 = a2auc_reg.reg_00|AUC_DAC_ENABLE;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ENABLE_REG, a2auc_reg.reg_00);
	a2auc_read(AUC_ENABLE_REG);
	}
}

static void a2auc_dac_off(void)
{
	//a2auc_reg.reg_00 = a2auc_read(AUC_ENABLE_REG);
	a2auc_reg.reg_00 = a2auc_reg.reg_00&(~((u32)AUC_DAC_ENABLE));
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_ENABLE_REG, a2auc_reg.reg_00);
	a2auc_read(AUC_ENABLE_REG);
	}
}

static void a2auc_dac_power_on(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f & (~((u32)AUC_DAC_CONTROL_DAC_PD_ALL)) ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
	a2auc_read(AUC_DAC_CONTROL_REG);
	}
}

static void a2auc_dac_power_down(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f= a2auc_reg.reg_0f | AUC_DAC_CONTROL_DAC_PD_ALL ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
	a2auc_read(AUC_DAC_CONTROL_REG);
	}
}

static void a2auc_dac_mute_on(void)
{
	a2auc_reg.reg_06 = AUC_I2S_GAIN_0_1;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
 	a2auc_write(AUC_I2S_REG,a2auc_reg.reg_06);
 	a2auc_read(AUC_I2S_REG);
 	}
}

static void a2auc_dac_mute_off(void)
{
	a2auc_reg.reg_06 = AUC_I2S_GAIN_1_1;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
 	a2auc_write(AUC_I2S_REG,a2auc_reg.reg_06);
 	a2auc_read(AUC_I2S_REG);
 	}
}

#if 0
static void a2auc_dac_dp_reset(void)
{
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
	a2auc_write(AUC_DP_RESET_REG, AUC_DP_RESET_DAC);
	a2auc_read(AUC_DP_RESET_REG);
	}
}
#endif

static void a2auc_volume_control(u8 volume)
{
	if (volume >= Gain_Table_Size) volume = Gain_Table_Size - 1;

	//a2auc_reg.reg_10 = (u32) a2auc_read(AUC_DAC_GAIN_REG);
	//a2auc_reg.reg_10 = a2auc_reg.reg_10 & (0xffffff00);
	//a2auc_reg.reg_10 = a2auc_reg.reg_10 | a2auc_pga_gain_table[volume];
	a2auc_reg.reg_10 = a2auc_pga_gain_table[volume];
	if (a2auc_check_state() != AUC_RESET_STATE) {
		a2auc_write(AUC_DAC_GAIN_REG, a2auc_reg.reg_10);
		a2auc_read(AUC_DAC_GAIN_REG);
	}
}

static void a2auc_line_power_on(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f & (~((u32)AUC_DAC_CONTROL_LINE_PD)) ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
		a2auc_read(AUC_DAC_CONTROL_REG);
	}
}
#if 0
static void a2auc_line_power_down(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f | AUC_DAC_CONTROL_LINE_PD ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
		a2auc_read(AUC_DAC_CONTROL_REG);
	}
}
#endif
static void a2auc_hp_power_on(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f & (~((u32)AUC_DAC_CONTROL_HP_PD)) ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
		a2auc_read(AUC_DAC_CONTROL_REG);
	}
}
#if 0
static void a2auc_hp_power_down(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f | AUC_DAC_CONTROL_HP_PD ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
		a2auc_read(AUC_DAC_CONTROL_REG);
	}
}
#endif
static void a2auc_sp_power_on(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f & (~((u32)AUC_DAC_CONTROL_SP_PD)) ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
		a2auc_read(AUC_DAC_CONTROL_REG);
	}
}
#if 0
static void a2auc_sp_power_down(void)
{
	//a2auc_reg.reg_0f = (u32) a2auc_read(AUC_DAC_CONTROL_REG);
	a2auc_reg.reg_0f = a2auc_reg.reg_0f | AUC_DAC_CONTROL_SP_PD ;
	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_write(AUC_DAC_CONTROL_REG, a2auc_reg.reg_0f);
		a2auc_read(AUC_DAC_CONTROL_REG);
	}
}
#endif


static void a2auc_set_adc_pga(u8 adc_pga)
{
	if (adc_pga > 0x1f )
	{
		printk("ADC PGA Setting Overflow, Maximum value is selected.");
		adc_pga = 0x1f;
	}

	if(a2auc_check_state()!=AUC_RESET_STATE)
	{
		a2auc_reg.reg_0e = (a2auc_reg.reg_0e&0xffe0) | adc_pga;
		a2auc_write(AUC_ADC_VGA_GAIN_REG, a2auc_reg.reg_0e);
		a2auc_read(AUC_ADC_VGA_GAIN_REG);
	}
}

static void a2auc_pwr_on(void)
{
	a2auc_adc_power_on();
	a2auc_adc_on();
	a2auc_dac_power_on();
	a2auc_dac_on();
}

static void a2auc_pwr_down(void)
{
	a2auc_adc_off();
	a2auc_adc_power_down();
	a2auc_dac_off();
	a2auc_dac_power_down();
}

static void a2auc_codec_init(void)
{
	/* Determine over sample rate */
	//a2auc_sfreq_conf();

	/* Enable De-Emphasis Filter */
	//a2auc_reg.reg_03 = (u32) a2auc_read(AUC_CONTROL_REG);
	//a2auc_reg.reg_03 = a2auc_reg.reg_03 | AUC_CONTROL_DEEMPHASIS_ON;
	//a2auc_write(AUC_CONTROL_REG, a2auc_reg.reg_03);
	a2auc_write(AUC_ENABLE_REG,0x0);
	a2auc_read(AUC_ENABLE_REG);

	a2auc_adc_off();
	a2auc_adc_power_down();
	a2auc_dac_off();
	a2auc_dac_power_down();
	a2auc_init_fiter();

	//a2auc_reg.reg_0d = a2auc_read(AUC_ADC_CONTROL_REG);
	a2auc_reg.reg_0d = (a2auc_reg.reg_0d | AUC_ADC_CONTROL_REG_OPT);
	a2auc_write(AUC_ADC_CONTROL_REG, a2auc_reg.reg_0d);
	a2auc_read(AUC_ADC_CONTROL_REG);

	a2auc_reg.reg_0e = AUC_ADC_VGA_GAIN_REG_OPT;
	a2auc_set_adc_pga(AUC_ADC_VGA_GAIN_REG_OPT);

	a2auc_volume_control(DAC_PGA_GAIN_0db);
	a2auc_dac_mute_on();
	a2auc_adc_mute_on();

	a2auc_dac_mute_off();
	a2auc_adc_mute_off();

	if(a2auc_read(AUC_ADC_CONTROL_REG)!=a2auc_reg.reg_0d)
	{
		printk("ERROR: AUC Initial Fail, Audio PLL Configuration Error\n");
//		A2AUC_ASSERT(0x1==0x0);
	}
	/* Enable Audio Codec ADC/DAC*/
}

static unsigned int a2auc_codec_read(struct snd_soc_codec *codec,
			unsigned int _reg)
{
	u32 reg = AUC_REG(_reg);
	return a2auc_read(reg);
}

static int a2auc_codec_write(struct snd_soc_codec *codec, unsigned int _reg,
			unsigned int value)
{
	u32 reg = AUC_REG(_reg);
	a2auc_write(reg, value);

	return 0;
}

static int a2auc_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	a2auc_pwr_on();

	return 0;
}

static void a2auc_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		a2auc_adc_off();
		a2auc_adc_power_down();
	} else {
		a2auc_dac_off();
		a2auc_dac_power_down();
	}
}

static int a2auc_digital_mute(struct snd_soc_dai *dai, int mute)
{
	if (mute)
		a2auc_dac_mute_on();
	else
		a2auc_dac_mute_off();

	return 0;
}

static int a2auc_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	int ret = 0;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int a2auc_set_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	/* a2auc_sfreq_conf */
	if(likely(div_id == A2AUC_CLKDIV_LRCLK)) {
		switch (div) {
		case AudioCodec_128xfs:
			a2auc_reg.reg_02 = AUC_OSR_128x;
			break;
		case AudioCodec_256xfs:
			a2auc_reg.reg_02 = AUC_OSR_256x;
			break;
		case AudioCodec_384xfs:
			a2auc_reg.reg_02 = AUC_OSR_384x;
			break;
		case AudioCodec_512xfs:
			a2auc_reg.reg_02 = AUC_OSR_512x;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	if(a2auc_check_state()!=AUC_RESET_STATE) {
		a2auc_write(AUC_OSR_REG, a2auc_reg.reg_02);
		a2auc_read(AUC_OSR_REG);
	}

	return 0;
}

static int a2auc_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON: /* full On */
		a2auc_pwr_on();
		a2auc_adc_mute_off();
		a2auc_dac_mute_off();
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY: /* Off, with power */
		a2auc_pwr_on();
		break;
	case SND_SOC_BIAS_OFF: /* Off, without power */
		/* everything off, dac mute, inactive */
		a2auc_dac_mute_on();
		a2auc_adc_mute_on();
		a2auc_pwr_down();
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static struct snd_soc_dai_ops ambarella_a2auc_dai_ops = {
	.startup = a2auc_startup,
	.shutdown = a2auc_shutdown,
	.digital_mute = a2auc_digital_mute,
	.set_fmt = a2auc_set_fmt,
	.set_clkdiv = a2auc_set_clkdiv,
};

static struct snd_soc_dai_driver ambarella_a2auc_dai = {
	.name = "a2auc-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &ambarella_a2auc_dai_ops,
};

static int a2auc_probe(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "A2AUC Audio Codec");

	a2auc_codec_init();

	/* power on device */
	a2auc_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	a2auc_line_power_on();
	a2auc_sp_power_on();
	a2auc_hp_power_on();

	snd_soc_add_controls(codec, a2auc_snd_controls,
				ARRAY_SIZE(a2auc_snd_controls));
	a2auc_add_widgets(codec);

	return 0;
}

static int a2auc_remove(struct snd_soc_codec *codec)
{
	a2auc_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

#ifdef CONFIG_PM
static int a2auc_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	a2auc_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int a2auc_resume(struct snd_soc_codec *codec)
{
#if 0 // FIXME recover all register?
	int i;
	u8 data[2];
	u16 *cache = codec->reg_cache;

	/* Sync reg_cache with the hardware */
	for (i = 0; i < ARRAY_SIZE(a2auc_reg); i++) {
		data[0] = (i << 1) | ((cache[i] >> 8) & 0x0001);
		data[1] = cache[i] & 0x00ff;
		codec->hw_write(codec->control_data, data, 2);
	}
#endif
	a2auc_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

#else
#define a2auc_suspend NULL
#define a2auc_resume NULL
#endif

static struct snd_soc_codec_driver soc_codec_dev_a2auc = {
	.probe =		a2auc_probe,
	.remove =		a2auc_remove,
	.suspend =		a2auc_suspend,
	.resume =		a2auc_resume,
	.read =			a2auc_codec_read,
	.write =		a2auc_codec_write,
	.set_bias_level =	a2auc_set_bias_level,
	.reg_cache_step =	4,
};

static int __devinit ambarella_a2auc_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_a2auc, &ambarella_a2auc_dai, 1);
}

static int __devexit ambarella_a2auc_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver ambarella_a2auc_codec_driver = {
	.probe		= ambarella_a2auc_codec_probe,
	.remove		= __devexit_p(ambarella_a2auc_codec_remove),
	.driver		= {
		.name	= "a2auc-codec",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_a2auc_init(void)
{
	return platform_driver_register(&ambarella_a2auc_codec_driver);
}
module_init(ambarella_a2auc_init);

static void __exit ambarella_a2auc_exit(void)
{
	platform_driver_unregister(&ambarella_a2auc_codec_driver);
}
module_exit(ambarella_a2auc_exit);


MODULE_DESCRIPTION("Ambarella A2AUC driver");
MODULE_AUTHOR("Eric Lee & Cao Rongrong");
MODULE_LICENSE("GPL");

