/**
 * system/src/peripheral/rct/i1.c
 *
 * History:
 *    2005/07/25 - [Charles Chiou] created file
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID
 *    2009/03/10 - [Chien-Yang Chen] integrated with hal.
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define ENABLE_DEBUG_MSG_RCT
#ifdef ENABLE_DEBUG_MSG_RCT
#define DEBUG_MSG	printk
#else
#define DEBUG_MSG(...)
#endif

#define PLL_VIDEO               0
#define PLL_VIDEO2              1
#define PLL_SO			3

static u32 g_ref_clk_hz;
//static u32 so_clk_freq_hz 	= 0;
static u32 spclk_freq_hz 	= 0;
static u32 vo_clk_freq_hz 	= 0;
static u32 vo2_clk_freq_hz 	= 0;
static u32 vo_clk_scaling 	= 0;
static u32 vo2_clk_scaling 	= 0;
//static u32 vin_clk_freq_hz	= 0;
static u32 vo_clk_src         	= VO_CLK_ONCHIP_PLL_27MHZ;
static u32 vo2_clk_src        	= VO_CLK_ONCHIP_PLL_27MHZ;
static u32 hdmi_clk_src         = HDMI_CLK_ONCHIP_PLL;


/* These options are for debug purpose. */
//#define __USE_DIRECT_RCT_PROGRAM	1

#ifdef __USE_DIRECT_RCT_PROGRAM


/**
 * VOUT RCT runtime object.
 */
struct vout_rct_obj_s {
	u32 	freq;
	u32 	video_ctrl;
	u32    	video_frac;
	u32    	scaler_pre;
	u32    	scaler_post;
	u32     os_ratio;
	u32	frac_100khz; /* Frac PLL change for 100KHz */
};

/**
 * RCT register settings for VOUT pll with Fref = 27MHz
 */
static struct vout_rct_obj_s G_vout_rct[] = {

	/* Freq     		Ctrl 	    FRAC        PRE  	 POST  	  0
						S_RATIO 	FRAC_100KHZ */
        {PLL_CLK_27MHZ,  	0x0a000108, 0x40000000,        0x0001, 0x000a, 0x0001},
        {PLL_CLK_74_25D1001MHZ, 0x09002108, 0x4D5CD5CE, 0x0001, 0x000a, 0x0001}, /* 74.25/1.001 MHz */
        {PLL_CLK_74_25MHZ,  	0x09002108, 0x50000000,	0x0001, 0x000a, 0x0001},
        {0x0, 	    		0x17100104, 0x0,        0x0001, 0x0018, 0x0001}  /* 27MHz as default */
};

#endif


void rct_pll_init(void)
{
	g_ref_clk_hz = (u32) amb_get_reference_clock_frequency(HAL_BASE_VP);

	/* Set audio clock to 12.288 Mhz. */
	if (amb_set_audio_clock_source(HAL_BASE_VP,
		AMB_PLL_REFERENCE_CLOCK_SOURCE_CLK_REF, 0) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_audio_clock_source() failed");
	}

	if (amb_set_audio_clock_frequency(HAL_BASE_VP, 12288000) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_audio_clock_frequency() failed");
	}
}

u32 get_apb_bus_freq_hz(void)
{
	return (u32)amb_get_apb_clock_frequency(HAL_BASE_VP);
}

u32 get_ahb_bus_freq_hz(void)
{
	return (u32)amb_get_ahb_clock_frequency(HAL_BASE_VP);
}

u32 get_core_bus_freq_hz(void)
{
	return (u32) amb_get_core_clock_frequency(HAL_BASE_VP);
}

/* arm clock is 2x or 4x times of idsp. */
u32 get_arm_bus_freq_hz(void)
{
	return (u32) amb_get_arm_clock_frequency(HAL_BASE_VP);
}

u32 get_dram_freq_hz(void)
{
	return (u32) amb_get_ddr_clock_frequency(HAL_BASE_VP);
}

u32 get_idsp_freq_hz(void)
{
	return (u32) amb_get_idsp_clock_frequency(HAL_BASE_VP);
}

u32 get_vout_freq_hz(void)
{
	return (u32) amb_get_vout_clock_frequency(HAL_BASE_VP);
}

u32 get_vout2_freq_hz(void)
{
	return (u32) amb_get_lcd_clock_frequency(HAL_BASE_VP);
}

u32 get_adc_freq_hz(void)
{
	return (u32) amb_get_adc_clock_frequency(HAL_BASE_VP);
}

void rct_set_adc_clk_src(int src)
{
	/* FIXME */
}

/**
 * UART 1 and UART 2 share the PLL
 */
u32 get_uart_freq_hz(void)
{
	return (u32) amb_get_uart_clock_frequency(HAL_BASE_VP);
}

u32 get_ssi_freq_hz(void)
{
	return (u32) amb_get_ssi_clock_frequency(HAL_BASE_VP);
}

u32 get_motor_freq_hz(void)
{
	return (u32) amb_get_motor_clock_frequency(HAL_BASE_VP);
}

u32 get_ms_freq_hz(void)
{
	return (u32) amb_get_ms_clock_frequency(HAL_BASE_VP);
}

u32 get_pwm_freq_hz(void)
{
	return (u32) amb_get_pwm_clock_frequency(HAL_BASE_VP);
}

u32 get_ir_freq_hz(void)
{
	return (u32) amb_get_ir_clock_frequency(HAL_BASE_VP);
}

u32 get_host_freq_hz(void)
{
	//return (u32) amb_get_host_clock_frequency(HAL_BASE_VP);
	return 0;
}

u32 get_sd_freq_hz(void)
{
	return (u32) amb_get_sd_clock_frequency(HAL_BASE_VP);
}

/**
 * Get sensor clock out
 */
u32 get_so_freq_hz(void)
{
	return (u32) amb_get_sensor_clock_frequency(HAL_BASE_VP);
}

/**
 * Get sensor clock out
 */
u32 get_spclk_freq_hz(void)
{
	return spclk_freq_hz;
}

void get_stepping_info(int *chip, int *major, int *minor)
{
	*chip  = 0x5;
	*major = 0x1;
	*minor = 0x0;
}

static u32 get_sm_boot_device(void)
{
	u32 rval = 0x0;

#if defined(FIRMWARE_CONTAINER_TYPE)
#if (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_SD)
		rval |= BOOT_FROM_SD;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_SDHC)
		rval |= BOOT_FROM_SDHC;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_MMC)
		rval |= BOOT_FROM_MMC;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_MOVINAND)
		rval |= BOOT_FROM_MOVINAND;
#endif
#endif
		return rval;
}

/* This function return the device boot from in different boot mode. */
u32 rct_boot_from(void)
{
	u32 rval = 0x0;
	u32 sm;
	amb_boot_type_t type;

	type = amb_get_boot_type(HAL_BASE_VP);

	if (type == AMB_NAND_BOOT) {
		rval |= BOOT_FROM_NAND;
	} else if (type == AMB_NOR_BOOT) {
		rval |= BOOT_FROM_NOR;
	} else if (type == AMB_SD_BOOT) {
		rval |= get_sm_boot_device();

	} else if (type == AMB_SSI_BOOT || type == AMB_XIP_BOOT) {
		rval |= BOOT_FROM_SPI;
		rval |= get_sm_boot_device();

	} else if (type == AMB_USB_BOOT) {
		sm = get_sm_boot_device();
		if (sm) {
			rval |= sm;
		} else {
			/* FIXME: */
			rval |= BOOT_FROM_NAND;
		}
	} else if (type == AMB_HIF_BOOT) {
		rval |= BOOT_FROM_HIF;
	}

#ifdef	RCT_BOOT_FROM
	/* The device boot from is specified by user. */
	rval = RCT_BOOT_FROM;
#endif

	return rval;
}

int rct_is_cf_trueide(void)
{
	return 0;
}

int rct_is_eth_enabled(void)
{
#if 1
	return 1;
#else
	amb_system_configuration_t cfg;

	cfg = amb_get_system_configuration(HAL_BASE_VP);
	if (cfg & AMB_SYSTEM_CONFIGURATION_ETHERNET_SELECTED)
		return 1;
	else
		return 0;
#endif
}

void rct_power_down(void)
{
#if 1
	/* This should be removed after standby mode is ready.*/
	/* ToDo: Remove this hack*/
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | ANA_PWR_POWER_DOWN);
#else
	amb_operating_mode_t amb_op_mode;

	amb_op_mode.performance	= AMB_PERFORMANCE_720P30;
	amb_op_mode.mode 	= AMB_OPERATING_MODE_STANDBY;
	amb_op_mode.usb_state 	= AMB_USB_OFF;
	amb_op_mode.hdmi_state 	= AMB_HDMI_OFF;

	if (amb_set_operating_mode(HAL_BASE_VP, &amb_op_mode) !=
						AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_operating_mode() failed");
	}
#endif
}

void rct_reset_chip(void)
{
	if (amb_reset_chip(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_chip() failed");
	}
}

void rct_reset_fio(void)
{
	if (amb_reset_all(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_fio() failed");
	}
}

void rct_reset_fio_only(void)
{
	if (amb_reset_fio(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_fio() failed");
	}
}

void rct_reset_cf(void)
{
	if (amb_reset_cf(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_cf() failed");
	}
}

void rct_reset_flash(void)
{
	if (amb_reset_flash(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_flash() failed");
	}
}

void rct_reset_xd(void)
{
	if (amb_reset_xd(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_xd() failed");
	}
}

void rct_reset_dma(void)
{
	u32 val;
	volatile int c;

	val = readl(I2S_24BITMUX_MODE_REG);
	val |= I2S_24BITMUX_MODE_RST_CHAN0;
	writel(I2S_24BITMUX_MODE_REG, val);

	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */

	val &= ~I2S_24BITMUX_MODE_RST_CHAN0;
	writel(I2S_24BITMUX_MODE_REG, val);

	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

void rct_set_uart_pll(void)
{
	/* set CG_UART = 1 that CLK_FRE = 24Mhz */
	if (amb_set_uart_clock_frequency(HAL_BASE_VP, 24000000) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_uart_clock_frequency() failed");
	}
}

void _rct_set_sd_pll(u32 freq_hz)
{
#define DUTY_CYCLE_CONTRL_ENABLE	0x01000000 /* Duty cycle correction */
	u32 scaler;
	u32 core_freq;

	K_ASSERT(freq_hz != 0);

	/* Scaler = core_freq *2 / desired_freq */
	core_freq = get_core_bus_freq_hz();
	scaler = ((core_freq << 1) / freq_hz) + 1;

	/* Sdclk = core_freq * 2 / Int_div */
	/* For example: Sdclk = 108 * 2 / 5 = 43.2 Mhz */
	/* For example: Sdclk = 121.5 * 2 / 5 = 48.6 Mhz */
	writel(SCALER_SD48_REG,
		(readl(SCALER_SD48_REG) & 0xffff0000) |
		(DUTY_CYCLE_CONTRL_ENABLE | scaler));

	DEBUG_MSG("SD Freq = %d, Set SCALER_SD48_REG 0x%x", freq_hz, scaler);
}

void rct_set_sd_pll(u32 freq_hz)
{
	if (amb_set_sd_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_sd_clock_frequency() failed");
	}
}

void rct_set_ir_pll(void)
{
	if (amb_set_ir_clock_frequency(HAL_BASE_VP, 13000) !=
						AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ir_clock_frequency() failed");
	}
}

void rct_set_ssi_pll(void)
{
	if (amb_set_ssi_clock_frequency(HAL_BASE_VP,
				54000000) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ssi_clock_frequency() failed");
	}
}

/**
 * SSI2 and SPI slave share the PLL
 */
u32 get_ssi2_freq_hz(void)
{
	return (u32) amb_get_ssi2_clock_frequency(HAL_BASE_VP);
}

void rct_set_ssi2_pll(void)
{
	 if (amb_set_ssi2_clock_frequency(HAL_BASE_VP,
				54000000) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ssi2_clock_frequency() failed");
	}
}

void rct_set_host_clk_freq_hz(u32 freq_hz)
{
#if 0
	if (amb_set_host_clock_frequency(HAL_BASE_VP, freq_hz) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("rct_set_host_clk_freq_hz() failed");
	}
#endif
}

/**
 * Configure sensor clock out
 */
void rct_set_so_freq_hz(u32 freq_hz)
{
	if (amb_set_sensor_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_sensor_clock_frequency() failed");
	}
}

/**
 *  Rescale the sensor clock frequency
 */
void rct_rescale_so_pclk_freq_hz(u32 scale)
{
	/* A5S does not support this function */
}

/**
 * The drivers of sensors and YUV devices shall call this function
 * to enable the pclk as the VOUT clock source
 */
void rct_set_so_pclk_freq_hz(u32 freq_hz)
{
        spclk_freq_hz = freq_hz;
}

/**
 * Configure sensor input clock source
 */
void rct_set_so_clk_src(u32 mode)
{
	u32 rval = AMB_HAL_SUCCESS;

	if (mode == AMB_SENSOR_CLOCK_PAD_OUTPUT_MODE) {
		rval = amb_set_sensor_clock_pad_mode(HAL_BASE_VP,
		 	(amb_sensor_clock_pad_mode_t)AMB_SENSOR_CLOCK_PAD_OUTPUT_MODE);
	} else if (mode == AMB_SENSOR_CLOCK_PAD_INPUT_MODE) {
		rval = amb_set_sensor_clock_pad_mode(HAL_BASE_VP,
		 	(amb_sensor_clock_pad_mode_t)AMB_SENSOR_CLOCK_PAD_INPUT_MODE);
	} else {
		DEBUG_MSG("rct_set_so_clk_src: mode %d not supported", mode);
	}

	if (rval  != AMB_HAL_SUCCESS) {
		DEBUG_MSG("rct_set_so_clk_src() failed");
	}
}

/**
 * Configure video clock source
 */
void rct_set_vout_clk_src(u32 clk_src)
{
	u32 rval = AMB_HAL_SUCCESS;
	vo_clk_src = clk_src;

	if (clk_src == VO_CLK_ONCHIP_PLL_27MHZ) {
		rval = amb_set_vout_clock_source(
				HAL_BASE_VP,
				AMB_PLL_REFERENCE_CLOCK_SOURCE_CLK_REF,
				0);
	} else if (clk_src == VO_CLK_ONCHIP_PLL_CLK_SI) {
		rval = amb_set_vout_clock_source(
				HAL_BASE_VP,
				AMB_PLL_REFERENCE_CLOCK_SOURCE_CLK_SI,
				spclk_freq_hz);
	} else if (clk_src == VO_CLK_ONCHIP_PLL_IDSP_SCLK) {
	 	rval = amb_set_vout_clock_source(
				HAL_BASE_VP,
				AMB_PLL_REFERENCE_CLOCK_SOURCE_LVDS_IDSP_SCLK,
				spclk_freq_hz);
	} else if (clk_src == VO_CLK_EXTERNAL) {
		rval = amb_set_vout_clock_source(
				HAL_BASE_VP,
				AMB_EXTERNAL_CLOCK_SOURCE,
				0);
	} else {
		DEBUG_MSG("rct_set_vout_clk_src() failed");
	}

	if ( rval != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_vout_clock_source() failed");
	}
}

/**
 *  Rescale the VOUT clock frequency in Hz
 *  Bit-31 is the sign bit and the others indicate the quantity.
 */
void rct_rescale_vout_clk_freq_hz(u32 scale)
{
	vo_clk_scaling = scale & 0x7fffffff;

	if (scale & 0x80000000) {
		vo_clk_freq_hz -= vo_clk_scaling;
	} else {
		vo_clk_freq_hz += vo_clk_scaling;
	}

	rct_set_vout_freq_hz(vo_clk_freq_hz);
}

/**
 *  Rescale the VOUT clock frequency
 */
u32 get_vout_clk_rescale_value(void)
{
	return vo_clk_scaling;
}

/**
 * Configure video2 clock source
 */
void rct_set_vout2_clk_src(u32 clk_src)
{
	u32 rval = AMB_HAL_SUCCESS;
	vo2_clk_src = clk_src;

	if (clk_src == VO_CLK_ONCHIP_PLL_27MHZ) {
		rval = amb_set_lcd_clock_source(
				HAL_BASE_VP,
				AMB_PLL_REFERENCE_CLOCK_SOURCE_CLK_REF,
				0);
	} else if (clk_src == VO_CLK_ONCHIP_PLL_CLK_SI) {
		rval = amb_set_lcd_clock_source(
				HAL_BASE_VP,
				AMB_PLL_REFERENCE_CLOCK_SOURCE_CLK_SI,
				spclk_freq_hz);
	} else if (clk_src == VO_CLK_ONCHIP_PLL_IDSP_SCLK) {
	 	rval = amb_set_lcd_clock_source(
				HAL_BASE_VP,
				AMB_PLL_REFERENCE_CLOCK_SOURCE_LVDS_IDSP_SCLK,
				spclk_freq_hz);
	} else if (clk_src == VO_CLK_EXTERNAL) {
		rval = amb_set_lcd_clock_source(
				HAL_BASE_VP,
				AMB_EXTERNAL_CLOCK_SOURCE,
				0);
	} else if (clk_src == VO2_CLK_SHARE_VOUT) {
		rval = amb_set_lcd_clock_source(
				HAL_BASE_VP,
				AMB_SHARE_VOUT_CLOCK,
				0);
	} else {
		DEBUG_MSG("rct_set_vout_clk_src() failed");
	}

	if ( rval != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_lcd_clock_source() failed");
	}
}


/**
 * Configure video clock out
 */
void rct_set_vout2_freq_hz(u32 freq_hz)
{
	if (amb_set_lcd_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_lcd_clock_frequency() failed");
	}
	vo_clk_freq_hz = freq_hz;
}

/**
 *  Rescale the VOUT clock frequency in Hz
 *  Bit-31 is the sign bit and the others indicate the quantity.
 */
void rct_rescale_vout2_clk_freq_hz(u32 scale)
{
	vo2_clk_scaling = scale & 0x7fffffff;

	if (scale & 0x80000000) {
		vo2_clk_freq_hz -= vo2_clk_scaling;
	} else {
		vo2_clk_freq_hz += vo2_clk_scaling;
	}

	rct_set_vout2_freq_hz(vo2_clk_freq_hz);
}

/**
 *  Rescale the VOUT clock frequency
 */
u32 get_vout2_clk_rescale_value(void)
{
	return vo2_clk_scaling;
}

/**
 * Configure video clock out
 */
void rct_set_vout_freq_hz(u32 freq_hz)
{
#ifdef __USE_DIRECT_RCT_PROGRAM
	int i;

	for (i = 0; ;i++) {
	        if ((G_vout_rct[i].freq == 0) ||
		    (G_vout_rct[i].freq == freq_hz))
			break;
	}

	/* Configure sensor clock out. The reference clock = 27 MHz */
	writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl & (~0x10));
	writel(PLL_VIDEO_FRAC_REG, G_vout_rct[i].video_frac);
	writel(SCALER_VIDEO_POST_REG, G_vout_rct[i].scaler_post);
	dly_tsk(1);
	writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl | 0x01);
	writel(SCALER_VIDEO_REG, G_vout_rct[i].scaler_pre);
	vo_clk_freq_hz = G_vout_rct[i].freq;

#else

	if (amb_set_vout_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_vout_clock_frequency() failed");
	}
#endif
}

/**
 * Configure stepping motor clock frequency
 */
void rct_set_motor_freq_hz(u32 freq_hz)
{
	if (amb_set_motor_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_motor_clock_frequency() failed");
	}
}

/**
 * Configure stepping motor clock frequency
 */
void rct_set_pwm_freq_hz(u32 freq_hz)
{
	if (amb_set_pwm_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_pwm_clock_frequency() failed");
	}
}

/* This function is just for USB device controller */
void rct_set_usb_ana_on(void)
{
	if (amb_set_usb_port1_state(HAL_BASE_VP, AMB_USB_ALWAYS_ON) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_port1_state() failed");
	}
}

void rct_suspend_usb(void)
{
	if (amb_set_usb_port1_state(HAL_BASE_VP, AMB_USB_SUSPEND) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_port1_state() failed");
	}

	if (amb_set_usb_port0_state(HAL_BASE_VP, AMB_USB_SUSPEND) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_port0_state() failed");
	}
}

/*
 * Used by low-level usb driver initialization
 */
void rct_set_usb_clk(void)
{
	/* FIXME, Set to internal 48MHz by default */
	if (amb_set_usb_port1_clock_source(HAL_BASE_VP, AMB_USB_CLK_CORE_48MHZ) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_port1_clock_source() failed\n");
	}

	/* FIXME, Set to internal 48MHz by default */
	if (amb_set_usb_port0_clock_source(HAL_BASE_VP, AMB_USB_CLK_CORE_48MHZ) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_port0_clock_source() failed\n");
	}
}

void rct_set_usb_ext_clk(void)
{
#if 0
	if (amb_set_usb_clock_source(HAL_BASE_VP, AMB_USB_CLK_EXT_48MHZ) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_clock_source() failed");
	}
#endif
}

void rct_ena_usb_int_clk(void)
{
#if 0
	if (amb_set_usb_clock_source(HAL_BASE_VP, AMB_USB_CLK_CORE_48MHZ) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_clock_source() failed");
	}
#endif
}

void rct_turn_off_usbp_pll(void)
{

}

void rct_turn_on_usbp_pll(void)
{

}

/*
 * Used by test_usb code
 */
void rct_set_usb_int_clk(void)
{
}

u32 read_usb_reg_setting(void)
{
	return 0;
}

/* called by prusb driver */
void _init_usb_pll(void)
{
	rct_set_usb_ana_on();
	/* Fixme: do we need to reduce the delay time ? */
	udelay(150);
}

void rct_usb_reset(void)
{
	if (amb_usb_device_soft_reset(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_usb_device_soft_reset() failed\r\n");
	}
}

/**
 * Select HDMI clock source
 */
void rct_set_hdmi_clk_src(u32 clk_src)
{
        hdmi_clk_src = clk_src;
        /* NOT supported by HAL */
}

void rct_set_hdmi_phy_freq_hz(u32 freq_hz)
{
	/* HDMI PHY is turned on/off via operating mode configuration */
}

int get_ssi_clk_src(void)
{
	/* FIXME: */
	return 0;
}

void rct_set_ssi_clk_src(int src)
{
	/* FIXME: */
}

/*
 * Config the mode of LVDS I/O pads
 */
void rct_set_vin_lvds_pad(int mode)
{
	u32 rval = AMB_HAL_SUCCESS;

	if (mode == VIN_LVDS_PAD_MODE_LVCMOS) {
		rval = amb_set_lvds_pad_mode(HAL_BASE_VP,
		 		        AMB_LVDS_PAD_MODE_LVCMOS);
	} else if (mode == VIN_LVDS_PAD_MODE_LVDS) {
		rval = amb_set_lvds_pad_mode(HAL_BASE_VP,
		 		      	     AMB_LVDS_PAD_MODE_LVDS);
	} else if (mode == VIN_LVDS_PAD_MODE_SLVS) {
		rval = amb_set_lvds_pad_mode(HAL_BASE_VP,
		 		      	     AMB_LVDS_PAD_MODE_SLVS);
	} else {
		DEBUG_MSG("rct_set_vin_lvds_pad: mode %d not supported", mode);
	}

	if (rval  != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_lvds_pad_mode() failed");
	}
}

void rct_enable_ms(void)
{
	if (amb_set_ms_status(HAL_BASE_VP,
				AMB_MS_ENABLE) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_status() failed");
	}
}

void rct_disable_ms(void)
{
	if (amb_set_ms_status(HAL_BASE_VP,
				AMB_MS_DISABLE) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_status() failed");
	}
}

void rct_set_ms_pll(u32 freq_hz)
{
	if (amb_set_ms_clock_frequency(HAL_BASE_VP,
				freq_hz) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_clock_frequency() failed");
	}
}

void rct_set_ms_delay(void)
{
#ifdef MS_READ_TIME_ADJUST
	if (amb_set_ms_read_delay(HAL_BASE_VP,
				MS_READ_TIME_ADJUST) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_read_delay() failed");
	}
#endif

#ifdef MS_DATA_OUTPUT_DELAY
	if (amb_set_ms_sd_output_delay(HAL_BASE_VP,
				MS_DATA_OUTPUT_DELAY) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_sd_output_delay() failed");
	}
#endif

#ifdef MS_DATA_INPUT_DELAY
	if (amb_set_ms_sd_input_delay(HAL_BASE_VP,
				MS_DATA_INPUT_DELAY) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_sd_input_delay() failed");
	}
#endif

#ifdef MS_SCLK_OUTPUT_DELAY
	if (amb_set_ms_sclk_delay(HAL_BASE_VP,
				MS_SCLK_OUTPUT_DELAY) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ms_sclk_delay() failed");
	}
#endif
}

void rct_set_dven_clk(u32 freq_hz)
{
	/* HAL always sets dven clock as 1000 Hz and MS driver uses this. */
	/* There is no need to support this API util we really need it */
}

void rct_set_adc_clk_freq_hz(u32 freq_hz)
{
	if (amb_set_adc_clock_frequency(HAL_BASE_VP, freq_hz) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_adc_clock_frequency() failed");
	}
}

#if 0
/**
 * IO pad control
 *
 * @params pad - PAD types
 * @params drv_strength - drive strength in mA
 */

/**
 * Set IO pad drive strength in mA
 */
int rct_set_io_pad_drv_strength(u32 pad, u32 drv_strength)
{
        u32 rval = AMB_HAL_SUCCESS;
	amb_ioctrl_drive_strength_t strength = AMB_IOCTRL_DRIVE_STRENGTH_2MA;

	if (drv_strength < 4) {
	        strength = AMB_IOCTRL_DRIVE_STRENGTH_2MA;
	} else if (drv_strength < 8) {
                strength = AMB_IOCTRL_DRIVE_STRENGTH_4MA;
	} else if (drv_strength < 12) {
                strength = AMB_IOCTRL_DRIVE_STRENGTH_8MA;
	} else {
                strength = AMB_IOCTRL_DRIVE_STRENGTH_12MA;
	}

	switch (pad) {
	/** PAD types */
	case IO_PAD_TYPE_MISC:
	        rval = amb_set_misc_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	  strength);
	        break;
	case IO_PAD_TYPE_SC:
	        rval = amb_set_sc_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	strength);
	        break;
	case IO_PAD_TYPE_STRIG:
	        rval = amb_set_strig_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	   strength);
	        break;
	case IO_PAD_TYPE_SMIO:
	        rval = amb_set_smio_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	  strength);
	        break;
	case IO_PAD_TYPE_VD0:
	        rval = amb_set_vd0_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	 strength);
	        break;
	case IO_PAD_TYPE_VD1:
	        rval = amb_set_vd1_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	 strength);
	        break;
	case IO_PAD_TYPE_SENSOR:
	        rval = amb_set_sensor_ioctrl_drive_strength(HAL_BASE_VP,
		 		        	 	    strength);
	        break;
	default:
                DEBUG_MSG("This PAD type is not supported!");
                return -1;
	};

	if (rval != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_*_ioctrl_drive_strength() failed");
		return -1;
	}

	return 0;
}

/**
 * Pull up or down PAD level
 */
int rct_set_io_pad_pull_up_down(u32 pad, u8 pull_up_down)
{
        u32 rval = AMB_HAL_SUCCESS;

	switch (pad) {
	/** PAD types */
	case IO_PAD_TYPE_MISC:
	        rval = amb_set_misc_ioctrl_pullupdown(HAL_BASE_VP,
		 		        	      pull_up_down);
	        break;
	case IO_PAD_TYPE_SC:
	        rval = amb_set_sc_ioctrl_pullupdown(HAL_BASE_VP,
						    pull_up_down);
	        break;
	case IO_PAD_TYPE_STRIG:
	        rval = amb_set_strig_ioctrl_pullupdown(HAL_BASE_VP,
						       pull_up_down);
	        break;
	case IO_PAD_TYPE_SMIO:
	        rval = amb_set_smio_ioctrl_pullupdown(HAL_BASE_VP,
						      pull_up_down);
	        break;
	case IO_PAD_TYPE_VD0:
	        rval = amb_set_vd0_ioctrl_pullupdown(HAL_BASE_VP,
						     pull_up_down);
	        break;
	case IO_PAD_TYPE_VD1:
	        rval = amb_set_vd1_ioctrl_pullupdown(HAL_BASE_VP,
						     pull_up_down);
	        break;
	case IO_PAD_TYPE_SENSOR:
	        rval = amb_set_sensor_ioctrl_pullupdown(HAL_BASE_VP,
						        pull_up_down);
	        break;
	default:
                DEBUG_MSG("This PAD type is not supported!");
                return -1;
	};

	if (rval != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_*_ioctrl_drive_strength() failed");
		return -1;
	}

	return 0;
}

/**
 * Get IO pad drive strength in mA
 */
int rct_get_io_pad_config(u32 pad, ioctrl_config_t *config)
{
	u32 rval = AMB_HAL_SUCCESS;
	amb_ioctrl_configuration_t ioctrl;

	switch (pad) {
	case IO_PAD_TYPE_MISC:
	        rval = amb_get_misc_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	case IO_PAD_TYPE_SC:
	        rval = amb_get_sc_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	case IO_PAD_TYPE_STRIG:
	        rval = amb_get_strig_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	case IO_PAD_TYPE_SMIO:
	        rval = amb_get_smio_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	case IO_PAD_TYPE_VD0:
	        rval = amb_get_vd0_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	case IO_PAD_TYPE_VD1:
	        rval = amb_get_vd1_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	case IO_PAD_TYPE_SENSOR:
	        rval = amb_get_sensor_ioctrl_configuration(HAL_BASE_VP, ioctrl);
	        break;
	default:
                DEBUG_MSG("This PAD type is not supported!");
                return -1;
	};

	if (rval != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_get_*_ioctrl_configuration() failed");
	}

        if (ioctrl.drive_strength == AMB_IOCTRL_DRIVE_STRENGTH_2MA) {
		config->drv_strength = 2;
	} else if (ioctrl.drive_strength == AMB_IOCTRL_DRIVE_STRENGTH_4MA) {
                config->drv_strength = 4;
	} else if (ioctrl.drive_strength == AMB_IOCTRL_DRIVE_STRENGTH_8MA) {
                config->drv_strength = 8;
	} else if (ioctrl.drive_strength == AMB_IOCTRL_DRIVE_STRENGTH_12MA) {
		config->drv_strength = 12;
	} else {
		config->drv_strength = 0xffffffff; /* Unknown value */
	}

	config->pull_up_down 	= (u8) ioctrl.pullupdown;
	config->input_type 	= (u8) ioctrl.input_type;
	config->slew_rate 	= (u8) ioctrl.slew_rate;

	return 0;
}
#endif
