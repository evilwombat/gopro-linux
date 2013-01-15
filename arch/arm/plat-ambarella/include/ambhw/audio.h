/*
 * ambhw/audio.h
 *
 * History:
 *	2008/02/18 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 */

#ifndef __AMBHW__AUDIO__
#define __AMBHW__AUDIO__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#if (CHIP_REV == A2)
#define AUDIO_CODEC_INSTANCES	1	/* Onchip audio codec */
#else
#define AUDIO_CODEC_INSTANCES	0
#endif

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

/**************************************/
/* On-chip Audio Codec */
/**************************************/

#if (AUDIO_CODEC_INSTANCES == 1)

#define AUC_ENABLE_OFFSET		0x000
#define AUC_DP_RESET_OFFSET		0x004
#define AUC_OSR_OFFSET                  0x008
#define AUC_CONTROL_OFFSET		0x00c
#define AUC_STATUS_OFFSET		0x010
#define AUC_CIC_OFFSET                  0x014
#define AUC_I2S_OFFSET                  0x018
#define AUC_DSM_GAIN_OFFSET		0x020
#define AUC_EMA_OFFSET                  0x024
#define AUC_ADC_CONTROL_OFFSET          0x034
#define AUC_ADC_VGA_GAIN_OFFSET		0x038
#define AUC_DAC_CONTROL_OFFSET          0x03c
#define AUC_DAC_GAIN_OFFSET		0x040
#define AUC_COEFF_START_DC_OFFSET	0x100
#define AUC_COEFF_START_IS_OFFSET	0x200
#define AUC_COEFF_START_PD_OFFSET	0x300
#define AUC_COEFF_START_HP_OFFSET	0x400
#define AUC_COEFF_START_WN_OFFSET	0x400
#define AUC_COEFF_START_DE_OFFSET	0x450

#define AUC_ENABLE_REG                  AUC_REG(0x000)
#define AUC_DP_RESET_REG		AUC_REG(0x004)
#define AUC_OSR_REG                     AUC_REG(0x008)
#define AUC_CONTROL_REG                 AUC_REG(0x00c)
#define AUC_STATUS_REG                  AUC_REG(0x010)
#define AUC_CIC_REG			AUC_REG(0x014)
#define AUC_I2S_REG			AUC_REG(0x018)
#define AUC_DSM_GAIN_REG		AUC_REG(0x020)
#define AUC_EMA_REG			AUC_REG(0x024)
#define AUC_ADC_CONTROL_REG		AUC_REG(0x034)
#define AUC_ADC_VGA_GAIN_REG		AUC_REG(0x038)
#define AUC_DAC_CONTROL_REG		AUC_REG(0x03c)
#define AUC_DAC_GAIN_REG		AUC_REG(0x040)
#define AUC_COEFF_START_DC_REG          AUC_REG(0x100)
#define AUC_COEFF_START_IS_REG		AUC_REG(0x200)
#define AUC_COEFF_START_PD_REG		AUC_REG(0x300)
#define AUC_COEFF_START_HP_REG		AUC_REG(0x400)
#define AUC_COEFF_START_WN_REG		AUC_REG(0x400)
#define AUC_COEFF_START_DE_REG		AUC_REG(0x450)
#endif

#endif
