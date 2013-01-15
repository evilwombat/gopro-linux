/*
 * arch/arm/plat-ambarella/include/plat/rct.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_RCT_H
#define __PLAT_AMBARELLA_RCT_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/***************************************************/
/* Functions for getting/setting system properties */
/* and anything else that involves the RCT module. */
/***************************************************/
extern void rct_pll_init(void);
extern u32 get_apb_bus_freq_hz(void);
extern u32 get_ahb_bus_freq_hz(void);
extern u32 get_core_bus_freq_hz(void);
extern u32 get_arm_bus_freq_hz(void);
extern u32 get_dram_freq_hz(void);
extern u32 get_idsp_freq_hz(void);
extern u32 get_adc_freq_hz(void);
extern u32 get_uart_freq_hz(void);
extern u32 get_ssi_freq_hz(void);
extern u32 get_motor_freq_hz(void);
extern u32 get_pwm_freq_hz(void);
extern u32 get_ir_freq_hz(void);
extern u32 get_host_freq_hz(void);
extern u32 get_sd_freq_hz(void);
extern u32 get_sd2_freq_hz(void);
extern u32 get_so_freq_hz(void);
extern u32 get_vout_freq_hz(void);
extern u32 get_vout2_freq_hz(void);
extern void get_stepping_info(int *chip, int *major, int *minor);
extern u32 get_ssi2_freq_hz(void);
extern u32 get_hdmi_phy_freq_hz(void);
extern u32 get_hdmi_cec_freq_hz(void);
extern u32 get_vin_freq_hz(void);
extern int get_ssi_clk_src(void);
extern u32 get_vout_clk_rescale_value(void);
extern u32 get_vout2_clk_rescale_value(void);
extern u32 rct_boot_from(void);
#define BOOT_FROM_BYPASS	0x00008000
#define BOOT_FROM_HIF		0x00004000
#define BOOT_FROM_NAND		0x00000001
#define BOOT_FROM_NOR		0x00000002
#define BOOT_FROM_ONENAND	0x00000100
#define BOOT_FROM_SNOR		0x00000200
#define BOOT_FROM_FLASH		(BOOT_FROM_NAND | BOOT_FROM_NOR | \
				 BOOT_FROM_ONENAND | BOOT_FROM_SNOR)
#define BOOT_FROM_NOR_FLASH	(BOOT_FROM_NOR | BOOT_FROM_SNOR)
#define BOOT_FROM_NAND_FLASH	(BOOT_FROM_NAND | BOOT_FROM_ONENAND)

#define BOOT_FROM_SPI		0x00000004
#define BOOT_FROM_SD		0x00000010
#define BOOT_FROM_SDHC		0x00000020
#define BOOT_FROM_MMC		0x00000040
#define BOOT_FROM_MOVINAND	0x00000080
#define BOOT_FROM_SDMMC		(BOOT_FROM_SD	| BOOT_FROM_SDHC	| \
				 BOOT_FROM_MMC	| BOOT_FROM_MOVINAND)

extern int rct_is_cf_trueide(void);
extern int rct_is_eth_enabled(void);
extern void rct_power_down(void);
extern void rct_reset_chip(void);
extern void rct_reset_fio(void);
extern void rct_reset_fio_only(void);
extern void rct_reset_cf(void);
extern void rct_reset_xd(void);
extern void rct_reset_flash(void);
extern void rct_reset_dma(void);
extern void rct_set_uart_pll(void);
extern void rct_set_sd_pll(u32 freq_hz);
extern void rct_set_sd2_pll(u32 freq_hz);
extern void rct_set_ir_pll(void);
extern void rct_set_motor_freq_hz(u32 freq_hz);
extern void rct_set_pwm_freq_hz(u32 freq_hz);
extern void rct_set_ssi_pll(void);
extern void rct_set_ssi2_pll(void);
extern void rct_set_host_clk_freq_hz(u32 freq_hz);
extern void rct_set_so_freq_hz(u32 freq_hz);
extern void rct_set_vout_freq_hz(u32 freq_hz);
extern void rct_set_hdmi_phy_freq_hz(u32 freq_hz);
extern void rct_set_ms_pll(u32 freq_hz);
extern u32 get_ms_freq_hz(void);
extern void rct_enable_ms(void);
extern void rct_disable_ms(void);
extern void rct_set_ms_delay(void);
extern void rct_set_dven_clk(u32 freq_hz);
extern void rct_set_adc_clk_freq_hz(u32 freq_hz);

/* *
 * Half the clock frequency of the PLL clk_so
 *
 * Param enable - 1: 1/2 clock frequecny, 0: original clock frequency
 */
extern void rct_set_half_clk_so_freq_hz(int enable);

/**
 * Set the VIN PLL
 *
 * @params ref_freq_hz - the physical clock rate from the sensor
 * @params freq_hz - the output clock rate of VIN PLL.
 *                   freq_hz = (ref_freq_hz (SDR) / bit_width) * # of active lanes
 *                   or
 *                   freq_hz = (ref_freq_hz (DDR) * 2 / bit_width) * # of active lanes
 */
extern void rct_set_vin_freq_hz(u32 ref_freq_hz, u32 freq_hz);

extern u32  rct_cal_vin_freq_hz_slvs(u32 dclk, u32 act_lanes,
						u32 pel_width, u32 ddr);

/**
 * Select ADC clock
 *
 * @param src -
 * 	0 - ref_clk / value of SCALER_ADC_REG
 * 	1 ¡V clk_au
 */
extern void rct_set_adc_clk_src(int src);

/**
 * Select the clock reference of SSI0
 *
 * @param src -
 * 	0 - ssi clock is derived from apb clock
 * 	1 ¡V ssi clock is derived from input pixel clk (spclk)
 */
extern void rct_set_ssi_clk_src(int src);

/**
 * The drivers of sensors and YUV devices shall call this function
 * to enable the pclk as the VOUT clock source
 */
extern void rct_set_so_pclk_freq_hz(u32 freq_hz);

/**
 * Configure sensor input clock source
 *
 * @param mode - the mode of input clock
 */
extern void rct_set_so_clk_src(u32 mode);

/**
 * Rescale the sensor clock frequency
 * 1 macrostep = 65536 microsteps
 * @params scale - the representation with sign bit. sign[31] + value[30..0]
 */
extern void rct_rescale_so_pclk_freq_hz(u32 scale);
extern u32 get_vout_clk_rescale_value(void);

/**
 * Select video clock source
 *
 * @param clk_src -
 *              VO_CLK_ONCHIP_PLL_27MHZ, 	On-chip PLL using 27MHz crystal
 *		VO_CLK_ONCHIP_PLL_SP_CLK,	On-chip PLL using SP_CLK
 *		VO_CLK_ONCHIP_PLL_CLK_SI,	On-chip PLL using CLK_SI
 *		VO_CLK_EXTERNAL,		External video clock source
 */
#define VO_CLK_ONCHIP_PLL_27MHZ   	0x0    	/* Default setting */
#define VO_CLK_ONCHIP_PLL_SP_CLK  	0x1
#define VO_CLK_ONCHIP_PLL_CLK_SI  	0x2
#define VO_CLK_EXTERNAL    	  	0x3
#define VO_CLK_ONCHIP_PLL_IDSP_SCLK	0x4
extern void rct_set_vout_clk_src(u32 clk_src);

/**
 * Rescale the VOUT clock frequency
 * 1 macrostep = 65536 microsteps
 * @params scale - the representation with sign bit. sign[31] + value[30..0]
 */
extern void rct_rescale_vout_clk_freq_hz(u32 scale);
extern u32 get_vout2_clk_rescale_value(void);

extern void rct_set_vout2_freq_hz(u32 freq_hz);
/**
 * Select video2 clock source
 *
 * @param clk_src -
 *              VO_CLK_ONCHIP_PLL_27MHZ, 	On-chip PLL using 27MHz crystal
 *		VO_CLK_ONCHIP_PLL_SP_CLK,	On-chip PLL using SP_CLK
 *		VO_CLK_ONCHIP_PLL_CLK_SI,	On-chip PLL using CLK_SI
 *		VO_CLK_EXTERNAL,		External video clock source
 */
#define VO2_CLK_SHARE_VOUT		0x5
extern void rct_set_vout2_clk_src(u32 clk_src);

/**
 * Rescale the VOUT clock frequency
 * 1 macrostep = 65536 microsteps
 * @params scale - the representation with sign bit. sign[31] + value[30..0]
 */
extern void rct_rescale_vout2_clk_freq_hz(u32 scale);

/**
 * Select HDMI clock source
 *
 * @param clk_src -
 *              HDMI_CLK_ONCHIP_PLL, 	On-chip PLL using 27MHz crystal
 *		HDMI_CLK_PHY_CLK_VO,	On-chip HDMI PHY clock
 *		HDMI_CLK_EXTERNAL,	External video clock source
 */

#define HDMI_CLK_ONCHIP_PLL   	0x0    	/* Default setting */
#define HDMI_CLK_PHY_CLK_VO  	0x1
#define HDMI_CLK_EXTERNAL  	0x2
extern void rct_set_hdmi_clk_src(u32 clk_src);

/**
 * Scale the HDMI reference clock from gclk_vo
 *
 * @param scalar - the scalar value
 */
extern void rct_set_gclk_vo_hdmi(u8 scalar);

/*
 * Config the mode of LVDS I/O pads
 */
#define VIN_LVDS_PAD_MODE_LVCMOS	0
#define VIN_LVDS_PAD_MODE_LVDS		1
#define VIN_LVDS_PAD_MODE_SLVS 		2
extern void rct_set_vin_lvds_pad(int mode);

#if (RCT_AUDIO_PLL_CONF_MODE > 0)
#if (RCT_AUDIO_PLL_USE_HAL_API == 0)
extern void rct_set_pll_frac_mode(void);
extern void rct_set_aud_ctrl2_reg(void);
extern void rct_set_audio_pll_reset(void);
#endif
#define AUC_CLK_ONCHIP_PLL_27MHZ				0x0 /* Default setting */
#define AUC_CLK_ONCHIP_PLL_SP_CLK				0x1
#define AUC_CLK_ONCHIP_PLL_CLK_SI				0x2
#define AUC_CLK_EXTERNAL						0x3
#define AUC_CLK_ONCHIP_PLL_LVDS_IDSP_SCLK	0x4

#define AUC_PLL_CLKRATE_NUM	16
#define AUC_PLL_CLKRATE_1350   	  	PLL_CLK_13_5MHZ
#define AUC_PLL_CLKRATE_2400   	  	PLL_CLK_24MHZ
#define AUC_PLL_CLKRATE_2700   	  	PLL_CLK_27MHZ
#define AUC_PLL_CLKRATE_2700_M1  	PLL_CLK_27M1001MHZ
#define AUC_PLL_CLKRATE_4800   	  	PLL_CLK_48MHZ
#define AUC_PLL_CLKRATE_7425_D1		PLL_CLK_74_25D1001MHZ
#define AUC_PLL_CLKRATE_7425   	  	PLL_CLK_74_25MHZ
#define AUC_PLL_CLKRATE_9069_D1		PLL_CLK_90_69D1001MHZ
#define AUC_PLL_CLKRATE_9069		PLL_CLK_90_69MHZ
#define AUC_PLL_CLKRATE_9600   	  	PLL_CLK_96MHZ
#define AUC_PLL_CLKRATE_9600_D1  	PLL_CLK_96D1001MHZ
#define AUC_PLL_CLKRATE_9900_D1  	PLL_CLK_99D1001MHZ
#define AUC_PLL_CLKRATE_9900   	  	PLL_CLK_99MHZ
#define AUC_PLL_CLKRATE_1485_D1		PLL_CLK_148_5D1001MHZ
#define AUC_PLL_CLKRATE_1485		PLL_CLK_148_5MHZ
#define AUC_PLL_CLKRATE_6500_D1	PLL_CLK_65D1001MHZ
extern void rct_set_audio_pll_fs(u8, u8);
#else
extern void rct_set_audio_pll_fs(u8);
#endif
extern u32 get_audio_freq_hz(void);

extern void _init_usb_pll(void);
extern void rct_set_usb_ana_on(void);
extern void rct_suspend_usb(void);
extern void rct_set_usb_clk(void);
extern void rct_set_usb_ext_clk(void);
extern void rct_set_usb_int_clk(void);
extern void rct_set_usb_debounce(void);
extern void rct_turn_off_usb_pll(void);
extern void rct_ena_usb_int_clk(void);
extern u32 read_usb_reg_setting(void);
extern void rct_usb_reset(void);
extern u32 read_pll_so_reg(void);
extern u32 read_cg_so_reg(void);
extern void write_pll_so_reg(u32 value);
extern void write_cg_so_reg(u32 value);


/******************************************/
/* PLL clock frequencies for SO and VOUT  */
/******************************************/
enum PLL_CLK_HZ {
	PLL_CLK_10D1001MHZ	=  9990010,
	PLL_CLK_10MHZ 		= 10000000,
	PLL_CLK_13D1001MHZ 	= 12987012,
	PLL_CLK_13MHZ 		= 13000000,
        PLL_CLK_13_5D1001MHZ 	= 13486513,
        PLL_CLK_13_5MHZ 	= 13500000,
        PLL_CLK_17_97515MHZ	= 17975154,
	PLL_CLK_17_97554MHZ	= 17975544,
	PLL_CLK_17_98201MHZ	= 17982018,
        PLL_CLK_18_44MHz	= 18440000,
        PLL_CLK_22_5MHZ 	= 22500000,
        PLL_CLK_23_9772MHZ     	= 23977223,
        PLL_CLK_23_9784MHZ     	= 23978422,
	PLL_CLK_23_99MHZ      	= 23998277,
	PLL_CLK_23_967MHZ	= 23967392,
	PLL_CLK_23_971MHZ	= 23971444,
        PLL_CLK_24D1001MHZ      = 23976024,
        PLL_CLK_23_996MHZ	= 23996483,
        PLL_CLK_24MHZ 		= 24000000,
        PLL_CLK_24M1001MHZ      = 24024000,
        PLL_CLK_24_072MHZ       = 24072001,
        PLL_CLK_24_3MHZ		= 24300000,
        PLL_CLK_24_54MHZ	= 24545430,
        PLL_CLK_25MHZ 		= 25000000,
	PLL_CLK_27D1001MHZ 	= 26973027,
	PLL_CLK_26_9823MHZ 	= 26982318,
	PLL_CLK_26_9485MHZ	= 26948544, /* - 1 LN at 60hz */
	PLL_CLK_26_9568MHZ	= 26956800, /* - 1 LN at 50hz */
	PLL_CLK_27MHZ 		= 27000000,
	PLL_CLK_27_0432MHZ	= 27043200, /* + 1 LN at 50hz */
	PLL_CLK_27_0514MHZ	= 27051402, /* + 1 LN at 60hz */
        PLL_CLK_27M1001MHZ 	= 27027000,
        PLL_CLK_30MHZ           = 30000000,
        PLL_CLK_27_1792MHZ 	= 27179210,
        PLL_CLK_32_5D1001MHZ	= 32467532,
        PLL_CLK_36D1001MHZ	= 35964036,
        PLL_CLK_36MHZ 		= 36000000,
        PLL_CLK_36_20MHZ 	= 36197802,
        PLL_CLK_36_23MHZ 	= 36234000,
	PLL_CLK_37_125D1001MHZ	= 37087912,
        PLL_CLK_37_125MHZ	= 37125000,
        PLL_CLK_42D1001MHZ     	= 41958042,
        PLL_CLK_42MHZ      	= 42000000,
        PLL_CLK_45MHZ      	= 45000000,
        PLL_CLK_48D1001MHZ      = 47952048,
        PLL_CLK_48MHZ 		= 48000000,
        PLL_CLK_48_6MHZ 	= 48600000,
        PLL_CLK_49_5D1001MHZ 	= 49450549,
        PLL_CLK_49_5MHZ 	= 49500000,
        PLL_CLK_54MHZ           = 54000000,
        PLL_CLK_54M1001MHZ 	= 54054000,
        PLL_CLK_60MHZ           = 60000000,
        PLL_CLK_60M1001MHZ      = 60060000,
        PLL_CLK_60_05MHz	= 60054545,
        PLL_CLK_60_16MHZ        = 60164835,
	PLL_CLK_60_18MHZ        = 60183566,
        PLL_CLK_60_29MHZ        = 60285794,
	PLL_CLK_60_33MHZ        = 60329670,
	PLL_CLK_60_35MHZ        = 60346080,
	PLL_CLK_60_39MHZ        = 60390000,
	PLL_CLK_60_48MHz	= 60480000,
	PLL_CLK_64D1001MHZ      = 63936064,
	PLL_CLK_64MHZ           = 64000000,
	PLL_CLK_65D1001MHZ      = 64935064,
	PLL_CLK_72D1001MHZ 	= 71928072,
        PLL_CLK_72MHZ 		= 72000000,
        PLL_CLK_74_25D1001MHZ 	= 74175824,
        PLL_CLK_74_25MHZ 	= 74250000,
        PLL_CLK_80MHZ 		= 80000000,
        PLL_CLK_90D1001MHZ	= 89910090,
        PLL_CLK_90MHZ 		= 90000000,
        PLL_CLK_90_62D1001MHZ	= 90525314,
        PLL_CLK_90_62MHZ	= 90615840,
        PLL_CLK_90_69D1001MHZ	= 90596763,
        PLL_CLK_90_69MHZ	= 90687360,
        PLL_CLK_95_993D1001MHZ  = 95896903,
	PLL_CLK_96D1001MHZ	= 95904096,
        PLL_CLK_96MHZ 		= 96000000,
        PLL_CLK_99D1001MHZ      = 98901099,
	PLL_CLK_99MHZ           = 99000000,
	PLL_CLK_99_18D1001MHZ   = 99081439,
	PLL_CLK_99_18MHZ        = 99180720,
	PLL_CLK_108MHZ 		= 108000000,
        PLL_CLK_148_5D1001MHZ 	= 148351648,
        PLL_CLK_148_5MHZ 	= 148500000,
        PLL_CLK_120MHZ 		= 120000000,
	PLL_CLK_144MHZ		= 144000000,
        PLL_CLK_150MHZ		= 150000000,
        PLL_CLK_156MHZ		= 156000000,
        PLL_CLK_160MHZ 		= 160000000,
	PLL_CLK_192MHZ 		= 192000000,
        PLL_CLK_216MHZ          = 216000000,
        PLL_CLK_230_4MHZ        = 230400000,
        PLL_CLK_240MHZ          = 240000000,
        PLL_CLK_288MHZ          = 288000000,
        PLL_CLK_320MHZ          = 320000000,
        PLL_CLK_384MHZ          = 384000000
};

/* ==========================================================================*/

/* ==========================================================================*/

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

