/*
 * ambhw/rct/a7.h
 *
 * History:
 *	2009/10/14 - [Allen Wang] created file
 *
 * Copyright (C) 2006-2009, Ambarella, Inc.
 */

#ifndef __AMBHW__RCT_A7_H__
#define __AMBHW__RCT_A7_H__

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/
#define RCT_DRAM_CLK_SRC_CORE		1
#define RCT_IDSP_CLK_SRC_CORE		1
#define RCT_ARM_CLK_SRC_IDSP		1
#define RCT_SUPPORT_PLL_DDR		1
#define RCT_SUPPORT_PLL_IDSP		1
#define RCT_MAX_DLL			4
#define RCT_MAX_DLL_CTRL		4
#define RCT_SUPPORT_ADC16_CTRL		1
#define RCT_SUPPORT_PLL_HDMI		1
#define RCT_SUPPORT_UNL_WDT_RST_ANAPWR	1 /* Use bit-7 of ANA_PWR_REG */

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

/*
 * Register definitions according to chip architecture
 */

/******************************************/
/* RCT (System Boot-up and Configuration) */
/******************************************/

/* Chip version independent registers */
#define CG_UART_OFFSET			0x38
#define CG_SSI_OFFSET			0x3c
#define CG_MOTOR_OFFSET			0x40
#define CG_IR_OFFSET			0x44
#define CG_HOST_OFFSET			0x48
#define ANA_PWR_OFFSET			0x50
#define SOFT_RESET_OFFSET		0x68
#define SOFT_OR_DLL_RESET_OFFSET	0X68
#define FIO_RESET_OFFSET		0x74
#define WDT_RST_L_OFFSET		0x78
#define CLK_DEBOUNCE_OFFSET		0x80

#define CG_UART_REG			RCT_REG(0x38)
#define CG_SSI_REG			RCT_REG(0x3c)
#define CG_MOTOR_REG			RCT_REG(0x40)
#define CG_IR_REG			RCT_REG(0x44)
#define CG_HOST_REG			RCT_REG(0x48)
#define ANA_PWR_REG			RCT_REG(0x50)
#define SOFT_RESET_REG			RCT_REG(0x68)
#define SOFT_OR_DLL_RESET_REG		RCT_REG(0x68)
#define FIO_RESET_REG			RCT_REG(0x74)
#define WDT_RST_L_REG			RCT_REG(0x78)
#define CLK_DEBOUNCE_REG		RCT_REG(0x80)

/* Chip version dependent registers */
#define PLL_CORE_CTRL_OFFSET		0x00
#define PLL_CORE_FRAC_OFFSET		0x04
#define SCALER_SD48_OFFSET		0x0c
#define SCALER_CORE_PRE_OFFSET		0x10
#define PLL_VIDEO_CTRL_OFFSET		0x14
#define PLL_VIDEO_FRAC_OFFSET		0x18
#define SCALER_VIDEO_OFFSET             0x1c
#define SCALER_VIDEO_POST_OFFSET	0xa0
#define PLL_SENSOR_CTRL_OFFSET		0x24
#define PLL_SENSOR_FRAC_OFFSET		0x28
#define SCALER_SENSOR_POST_OFFSET	0x30
#define SYS_CONFIG_OFFSET		0x34
#define SCALER_SENSOR_PRE_OFFSET	0x4c
#define PLL_AUDIO_CTRL_OFFSET		0x54
#define PLL_AUDIO_FRAC_OFFSET		0x58
#define SCALER_AUDIO_OFFSET		0x5c
#define SCALER_AUDIO_PRE_OFFSET		0x60
#define PLL_USB_CTRL_OFFSET		0x6c
#define PLL_USB_FRAC_OFFSET		0x70
#define SCALER_USB_OFFSET		0x7c
#define CG_PWM_OFFSET			0x84
#define USB_REFCLK_OFFSET		0x88
#define CKEN_VDSP_OFFSET		0x8c
#define DLL0_OFFSET			0x90
#define DLL1_OFFSET			0x94
#define SCALER_ADC_OFFSET		0x9c
#define CLK_REF_AU_EXTERNAL_OFFSET	0xa4
#define USE_EXTERNAL_CLK_AU_OFFSET	0xa8
#define CLK_REG_VIDEO_EXTERNAL_OFFSET	0xac
#define USE_EXTERNAL_VD_CLK_OFFSET	0xb0
#define USE_CLK_SI_4_AU_OFFSET		0xb4
#define USE_CLK_SI_4_VO_OFFSET		0xb8
#define USE_CLK_SI_INPUT_MODE_OFFSET	0xbc

#define PLL_CORE_CTRL_REG		RCT_REG(0x00)
#define PLL_CORE_FRAC_REG		RCT_REG(0x04)
#define SCALER_SD48_REG			RCT_REG(0x0c)
#define SCALER_CORE_PRE_REG		RCT_REG(0x10)
#define PLL_VIDEO_CTRL_REG		RCT_REG(0x14)
#define PLL_VIDEO_FRAC_REG		RCT_REG(0x18)
#define SCALER_VIDEO_REG                RCT_REG(0x1c)
#define SCALER_VIDEO_POST_REG		RCT_REG(0xa0)
#define PLL_SENSOR_CTRL_REG		RCT_REG(0x24)
#define PLL_SENSOR_FRAC_REG		RCT_REG(0x28)
#define SCALER_SENSOR_POST_REG		RCT_REG(0x30)
#define SYS_CONFIG_REG			RCT_REG(0x34)
#define SCALER_SENSOR_PRE_REG		RCT_REG(0x4c)
#define PLL_AUDIO_CTRL_REG		RCT_REG(0x54)
#define PLL_AUDIO_FRAC_REG		RCT_REG(0x58)
#define SCALER_AUDIO_REG		RCT_REG(0x5c)
#define SCALER_AUDIO_PRE_REG		RCT_REG(0x60)
#define PLL_USB_CTRL_REG		RCT_REG(0x6c)
#define PLL_USB_FRAC_REG		RCT_REG(0x70)
#define SCALER_USB_REG			RCT_REG(0x7c)
#define CG_PWM_REG			RCT_REG(0x84)
#define USB_REFCLK_REG			RCT_REG(0x88)
#define CKEN_VDSP_REG			RCT_REG(0x8c)
#define DLL0_REG			RCT_REG(0x90)
#define DLL1_REG			RCT_REG(0x94)
#define SCALER_ADC_REG			RCT_REG(0x9c)
#define CLK_REF_AU_EXTERNAL_REG		RCT_REG(0xa4)
#define USE_EXTERNAL_CLK_AU_REG		RCT_REG(0xa8)
#define CLK_REF_VIDEO_EXTERNAL_REG	RCT_REG(0xac)
#define USE_EXTERNAL_VD_CLK_REG		RCT_REG(0xb0)
#define USE_CLK_SI_4_AU_REG		RCT_REG(0xb4)
#define USE_CLK_SI_4_VO_REG		RCT_REG(0xb8)
#define USE_CLK_SI_INPUT_MODE_REG	RCT_REG(0xbc)

/* PLL_XXX_CTRL_REG */
#define PLL_CTRL_VAL(intprog, sout, sdiv, valwe)	\
	(((intprog & 0x7f) << 24)	|	\
	 ((sout & 0xf) << 16)		| 	\
	 ((sdiv & 0xf) << 12)		|	\
	 (valwe & 0xfff))

#define PLL_CTRL_INTPROG(x)	((x >> 24) & 0x7f)
#define PLL_CTRL_SOUT(x)	((x >> 16) & 0xf)
#define PLL_CTRL_SDIV(x)	((x >> 12) & 0xf)
#define PLL_CTRL_VALWE(x)	(x & 0xfff)

/* PLL_XXX_FRAC_REG */
#define PLL_FRAC_VAL(frac)	(frac & 0x7fffffff)

/* SCALER_XXX_PRE_REG */
#define SCALER_PRE_VAL(dcc, p, n)		\
	(((dcc & 0x1) << 24)		|	\
	 ((p & 0x1f) << 16)		|	\
	 (n & 0xffff))

#define SCALER_PRE_P(x)		((x >> 16) & 0x1f)
#define SCALER_PRE_N(x)		(x & 0xffff)
#define SCALER_POST(x)		(x & 0xf)

#define HDMI_CTRL_OFFSET		0x08	/* A3 and later */
#define HDMI_CTRL_REG			RCT_REG(0x08)

#define PLL_LOCK_OFFSET			0x2c
#define PLL_LOCK_REG			RCT_REG(0x2c)

/* PLL_LOCK_REG */
#define PLL_LOCK_VIN	        (0x1 << 9)
#define PLL_LOCK_HDMI         	(0x1 << 8)
#define PLL_LOCK_AUDIO  	(0x1 << 7)
#define PLL_LOCK_CORE	        (0x1 << 6)
#define PLL_LOCK_DDR            (0x1 << 5)
#define PLL_LOCK_IDSP           (0x1 << 4)
#define PLL_LOCK_SENSOR         (0x1 << 3)
#define PLL_LOCK_USB            (0x1 << 2)
#define PLL_LOCK_VIDEO          (0x1 << 1)
#define PLL_LOCK_VIDEO2         (0x1 << 0)

#define SENSOR_PAD_CTRL_OFFSET		0x98
#define SENSOR_PAD_CTRL_REG		RCT_REG(0x98)

/* SENSOR_PAD_CTRL_REG */
#define SENSOR_PAD_CTRL_PMEMIO_A6	0x40
#define SENSOR_PAD_CTRL_PMEMIO_A2	0x20
#define SENSOR_PAD_CTRL_PMEMIO_DS	0x10
#define SENSOR_PAD_CTRL_PMEMIO_LVCMOS	0x08
#define SENSOR_PAD_CTRL_PMEMIO_S1	0x04
#define SENSOR_PAD_CTRL_PMEMIO_S0	0x02
#define SENSOR_PAD_CTRL_PMEMIO_PWD	0x01

#define PLL_VIDEO2_CTRL_OFFSET		0xc0
#define PLL_VIDEO2_FRAC_OFFSET		0xc4
#define SCALER_VIDEO2_OFFSET		0xc8
#define SCALER_VIDEO2_POST_OFFSET	0xcc
#define USE_CLK_SI_4_VO2_OFFSET		0xd0
#define USE_EXTERNAL_VD2_CLK_OFFSET	0xd4
#define CLK_REF_VIDEO2_EXTERNAL_OFFSET	0xd8

#define PLL_VIDEO2_CTRL_REG		RCT_REG(0xc0)
#define PLL_VIDEO2_FRAC_REG		RCT_REG(0xc4)
#define SCALER_VIDEO2_REG		RCT_REG(0xc8)
#define SCALER_VIDEO2_POST_REG		RCT_REG(0xcc)
#define USE_CLK_SI_4_VO2_REG		RCT_REG(0xd0)
#define USE_EXTERNAL_VD2_CLK_REG	RCT_REG(0xd4)
#define CLK_REF_VIDEO2_EXTERNAL_REG	RCT_REG(0xd8)

#define PLL_IDSP_CTRL_OFFSET		0xe4
#define PLL_IDSP_FRAC_OFFSET		0xe8
#define PLL_IDSP_CTRL_REG		RCT_REG(0xe4)
#define PLL_IDSP_FRAC_REG		RCT_REG(0xe8)

#define CG_SSI2_OFFSET			0xec
#define CG_SSI2_REG			RCT_REG(0xec)

#define PLL_DDR_CTRL_OFFSET		0xdc
#define PLL_DDR_FRAC_OFFSET		0xe0

#define PLL_DDR_CTRL_REG		RCT_REG(0xdc)
#define PLL_DDR_FRAC_REG		RCT_REG(0xe0)

/* DLL0_REG */
#define DLL0_VAL(madj_0,adj_0_0,adj_0_1)		\
	(((madj_0) & 0xff)           |			\
	 (((adj_0_0) & 0xff) << 8)   |			\
	 (((adj_0_1) & 0xff) << 16)) |

/* DLL1_REG */
#define DLL1_VAL(madj_1,adj_1_0,adj_1_1)		\
	(((madj_1) & 0xff)           |			\
	 (((adj_1_0) & 0xff) << 8)   |			\
	 (((adj_1_1) & 0xff) << 16)) |

#define DLL2_OFFSET			0xf0
#define DLL3_OFFSET			0xf4
#define DLL2_REG			RCT_REG(0xf0)
#define DLL3_REG			RCT_REG(0xf4)

#define LVDS_LVCMOS_OFFSET		0xf8
#define LVDS_ASYNC_OFFSET		0xfc
#define LVDS_LVCMOS_REG			RCT_REG(0xf8)
#define LVDS_ASYNC_REG			RCT_REG(0xfc)

#define PLL_CORE_CTRL2_OFFSET		0x100
#define PLL_CORE_CTRL3_OFFSET		0x104
#define PLL_IDSP_CTRL2_OFFSET		0x108
#define PLL_IDSP_CTRL3_OFFSET		0x10c
#define PLL_DDR_CTRL2_OFFSET		0x110
#define PLL_DDR_CTRL3_OFFSET		0x114
#define PLL_SENSOR_CTRL2_OFFSET		0x11c
#define PLL_SENSOR_CTRL3_OFFSET		0x120
#define PLL_AUDIO_CTRL2_OFFSET		0x124
#define PLL_AUDIO_CTRL3_OFFSET		0x12c
#define PLL_VIDEO_CTRL2_OFFSET		0x130
#define PLL_VIDEO_CTRL3_OFFSET		0x134
#define PLL_VIDEO2_CTRL2_OFFSET		0x13c
#define PLL_VIDEO2_CTRL3_OFFSET		0x140
#define PLL_USB_CTRL2_OFFSET		0x144
#define PLL_USB_CTRL3_OFFSET		0x14c
#define PLL_HDMI_CTRL2_OFFSET		0x150
#define PLL_HDMI_CTRL3_OFFSET		0x154
#define PLL_VIN_CTRL2_OFFSET		0x1b8
#define PLL_VIN_CTRL3_OFFSET		0x1bc

#define PLL_CORE_CTRL2_REG		RCT_REG(0x100)
#define PLL_CORE_CTRL3_REG		RCT_REG(0x104)
#define PLL_IDSP_CTRL2_REG		RCT_REG(0x108)
#define PLL_IDSP_CTRL3_REG		RCT_REG(0x10c)
#define PLL_DDR_CTRL2_REG		RCT_REG(0x110)
#define PLL_DDR_CTRL3_REG		RCT_REG(0x114)
#define SCALER_CORE_POST_REG		RCT_REG(0x118)
#define PLL_SENSOR_CTRL2_REG		RCT_REG(0x11c)
#define PLL_SENSOR_CTRL3_REG		RCT_REG(0x120)
#define PLL_AUDIO_CTRL2_REG		RCT_REG(0x124)
#define PLL_AUDIO_CTRL3_REG		RCT_REG(0x12c)
#define PLL_VIDEO_CTRL2_REG		RCT_REG(0x130)
#define PLL_VIDEO_CTRL3_REG		RCT_REG(0x134)
#define PLL_VIDEO2_CTRL2_REG		RCT_REG(0x13c)
#define PLL_VIDEO2_CTRL3_REG		RCT_REG(0x140)
#define PLL_USB_CTRL2_REG		RCT_REG(0x144)
#define PLL_USB_CTRL3_REG		RCT_REG(0x14c)
#define PLL_HDMI_CTRL2_REG		RCT_REG(0x150)
#define PLL_HDMI_CTRL3_REG		RCT_REG(0x154)
#define PLL_VIN_CTRL2_REG		RCT_REG(0x1b8)
#define PLL_VIN_CTRL3_REG		RCT_REG(0x1bc)

#define DLL_CTRL_SEL_OFFSET		0x158
#define DLL_OCD_BITS_OFFSET		0x15c
#define DDRIO_CALIB_OFFSET		0x160
#define DLL_CTRL_SEL_REG		RCT_REG(0x158)
#define DLL_OCD_BITS_REG		RCT_REG(0x15c)
#define DDRIO_CALIB_REG			RCT_REG(0x160)

#define PLL_HDMI_CTRL_OFFSET		0x164
#define PLL_HDMI_FRAC_OFFSET		0x168
#define SCALER_HDMI_POST_OFFSET		0x16c
#define SCALER_HDMI_PRE_OFFSET		0x170

#define PLL_HDMI_CTRL_REG		RCT_REG(0x164)
#define PLL_HDMI_FRAC_REG		RCT_REG(0x168)
#define SCALER_HDMI_POST_REG		RCT_REG(0x16c)
#define SCALER_HDMI_PRE_REG		RCT_REG(0x170)

#define PLL_VIN_CTRL_OFFSET		0x1a8
#define PLL_VIN_FRAC_OFFSET		0x1ac
#define SCALER_VIN_POST_OFFSET		0x1b0
#define SCALER_VIN_PRE_OFFSET		0x1b4

#define PLL_VIN_CTRL_REG		RCT_REG(0x1a8)
#define PLL_VIN_FRAC_REG		RCT_REG(0x1ac)
#define SCALER_VIN_POST_REG		RCT_REG(0x1b0)
#define SCALER_VIN_PRE_REG		RCT_REG(0x1b4)

#define ADC16_CTRL_OFFSET		0x198
#define ADC16_CTRL_REG			RCT_REG(0x198)

#define CLK_REF_SSI_OFFSET		0x19c
#define CLK_REF_SSI_REG			RCT_REG(0x19c)

#define T2V_CTRL_OFFSET			0x1a0
#define T2V_CTRL_REG			RCT_REG(0x1a0)

#define RNG_CTRL_OFFSET			0x1a4
#define RNG_CTRL_REG			RCT_REG(0x1a4)

#define DLL_CTRL_SEL0_OFFSET		0x23c
#define DLL_CTRL_SEL1_OFFSET		0x240
#define DLL_CTRL_SEL2_OFFSET		0x244
#define DLL_CTRL_SEL3_OFFSET		0x248

#define DLL_CTRL_SEL0_REG		RCT_REG(0x23c)
#define DLL_CTRL_SEL1_REG		RCT_REG(0x240)
#define DLL_CTRL_SEL2_REG		RCT_REG(0x244)
#define DLL_CTRL_SEL3_REG		RCT_REG(0x248)

#define CORE_CLK_RATIO_1X_OFFSET	0x24c
#define CORE_CLK_RATIO_1X_REG		RCT_REG(0x24c)

/* Memory stick */
#define CG_DVEN_OFFSET			0x1c8
#define SCALER_MS_OFFSET		0x1cc
#define MS_DELAY_CTRL_OFFSET		0x1d0
#define CG_DVEN_REG			RCT_REG(0x1c8)
#define SCALER_MS_REG			RCT_REG(0x1cc)
#define MS_DELAY_CTRL_REG		RCT_REG(0x1d0)

#define USE_COMMON_VO_CLK_OFFSET	0x1d4
#define USE_COMMON_VO_CLK_REG		RCT_REG(0x1d4)

#define DISABLE_EXT_BYPASS_OFFSET	0x1e4
#define DISABLE_EXT_BYPASS_REG		RCT_REG(0x1e4)

/* ANA_PWR_REG */
#define ANA_PWR_DLL_POWER_DOWN		0x80
#define ANA_PWR_PMEMIO_S1		0x40
#define ANA_PWR_POWER_DOWN		0x20
#define ANA_PWR_NEVER_SUSPEND_USBP	0x04
#define ANA_PWR_USB_SUSPEND		0x02

/* FIO_RESET_REG */
#define FIO_RESET_FIO_RST		0x00000008
#define FIO_RESET_CF_RST		0x00000004
#define FIO_RESET_XD_RST		0x00000002
#define FIO_RESET_FLASH_RST		0x00000001

/* USBP_REFCLK_REG */
#define USBP_REFCLK_TX_TUNE_11		0x0c00
#define USBP_REFCLK_TX_TUNE_10		0x0800
#define USBP_REFCLK_TX_TUNE_01		0x0400
#define USBP_REFCLK_TX_TUNE_00		0x0000
#define USBP_REFCLK_WAK_PULLDOWN_EN	0x0200
#define USBP_REFCLK_DP_PULLUP_ESD	0x0100
#define USBP_REFCLK_ESD_TEST_MODE	0x0080
#define USBP_REFCLK_COMMON_ON_N		0x0040
#define USBP_REFCLK_CLK48M_SEL		0x0020
#define USBP_REFCLK_CLK_SEL_11		0x000c
#define USBP_REFCLK_CLK_SEL_10		0x0008
#define USBP_REFCLK_CLK_SEL_01		0x0004
#define USBP_REFCLK_CLK_SEL_00		0x0000
#define USBP_REFCLK_XO_EXT_CLK_ENBN	0x0002
#define USBP_REFCLK_XO_REFCLK_ENB	0x0001

/* CKEN_VDSP_REG */
#define CKEN_VDSP_SMEM			0x08
#define CKEN_VDSP_CODE			0x04
#define CKEN_VDSP_TSFM			0x02
#define CKEN_VDSP_MEMD			0x01


/*
 * SYS_CONFIG_REG
 */
#define SYS_CONFIG_USB_PHY_EXT_CRYSTAL	0x00800000
#define SYS_CONFIG_RCT_SMEM_EFUSE_DISABLE 0x00400000
#define SYS_CONFIG_RCT_AHB_HIF_SECURE	0x00200000
#define SYS_CONFIG_RDY_PL		0x00100000
	/*
	 * sets polarity of hif_rdy signal
	 * 0 -> hif_rdy active low
	 * 1 -> hif_rdy active high
	 */
#define SYS_CONFIG_HIF_TYPE		0x00080000
#define SYS_CONFIG_HIF_PORT_SIZE	0x00040000
#define SYS_CONFIG_HIF_EN		0x00020000
#define SYS_CONFIG_SPI_BOOT		0x00010000

#define SYS_CONFIG_RMII_SEL		0x00008000
#define SYS_CONFIG_			0x00006000
	/*
	 * 00 - full FME address range in use
	 * 01 - FMEM BIST clk enable forced on
	 * 10 -only lower half of FMEM used(A6)
	 * 11 -only upper half of FMEM used(A6)
	 */
#define SYS_CONFIG_EMA_SEL		0x00001000
#define SYS_CONFIG_SD_BOOT		0x00000800
#define SYS_CONFIG_FLASH_BOOT		0x00000400
	/*
	 * 0 : Boot with USB (ARM code on internal ROM).
	 * 1 : Boot using FIO controller (NAND or NOR flash).
	 */
#define SYS_CONFIG_FAST_BOOT		0x00000200
#define SYS_CONFIG_BOOT_BYPASS		0x00000100
	/*
	 * 0: ARM waits for FIO 2KB code fetch before execution.
	 * 1: ARM does not wait for FIO to fetch 2KB code before execution.
	 */
#define SYS_CONFIG_ENET_SEL		0x00000080 /* 0: disable, 1: enable */
#define SYS_CONFIG_NAND_READ_CONFIRM	0x00000040 /* 0: use, 1: doesn't use */
#define SYS_CONFIG_NAND_FLASH_PAGE	0x00000020 /* 0: 512Byte, 1: 2K Byte */
#define SYS_CONFIG_PLL_UNLOCK_TRIG_RESET 0x00000010
	/*
	 * 0: disable generating grst when core pll out of lock.(default)
	 * 1: enable generating grst when core pll out of lock.
	 */
						   /* Idsp  / Core  / DDR   */
#define SYS_CONFIG_CLOCK_CONFIG_7	0x0000000e /* 108   / 108   / 216   */
#define SYS_CONFIG_CLOCK_CONFIG_6	0x0000000c /* 256.5 / 243   / 337.5 */
#define SYS_CONFIG_CLOCK_CONFIG_5	0x0000000a /* 256.5 / 216   / 337.5 */
#define SYS_CONFIG_CLOCK_CONFIG_4	0x00000008 /* 256.5 / 216   / 337.5 */
#define SYS_CONFIG_CLOCK_CONFIG_3	0x00000006 /* 202.5 / 202.5 / 337.5 */
#define SYS_CONFIG_CLOCK_CONFIG_2	0x00000004 /* 148.5 / 148.5 / 337.5 */
#define SYS_CONFIG_CLOCK_CONFIG_1	0x00000002 /* 148.5 / 148.5 / 337.5 */
#define SYS_CONFIG_CLOCK_CONFIG_0	0x00000000 /* 216   / 216   / 432   */
#define SYS_CONFIG_BOOTMEDIA		0x00000001 /* 0: NAND, 1: NOR */

/*
 * Core frequency values
 */
#define PLL_CORE_108MHZ_VAL	0x07011100
#define PLL_CORE_148MHZ_VAL	0x0a011100
#define PLL_CORE_202MHZ_VAL	0x0e011100
#define PLL_CORE_216MHZ_VAL	0x0f000100
#define PLL_CORE_243MHZ_VAL	0x11000100

#if	defined(FIX_CORE_108MHZ)
#define	EXPECT_PLL_CORE_VAL	PLL_CORE_108MHZ_VAL
#elif	defined(FIX_CORE_148MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_148MHZ_VAL
#elif	defined(FIX_CORE_202MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_202MHZ_VAL
#elif	defined(FIX_CORE_216MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_216MHZ_VAL
#elif	defined(FIX_CORE_243MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_243MHZ_VAL
#else

#if	defined(PWC_CORE_108MHZ)
#define	EXPECT_PLL_CORE_VAL	PLL_CORE_108MHZ_VAL
#elif	defined(PWC_CORE_148MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_148MHZ_VAL
#elif	defined(PWC_CORE_202MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_202MHZ_VAL
#elif	defined(PWC_CORE_216MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_216MHZ_VAL
#elif	defined(PWC_CORE_243MHZ)
#define EXPECT_PLL_CORE_VAL	PLL_CORE_243MHZ_VAL
#else
#define EXPECT_PLL_CORE_VAL	0xffffffff
#endif

#endif

/*
 * IDSP frequency values
 */
#define PLL_IDSP_108MHZ_VAL	0x07011100
#define PLL_IDSP_148MHZ_VAL	0x0a011100
#define PLL_IDSP_202MHZ_VAL	0x0e011100
#define PLL_IDSP_216MHZ_VAL	0x0f000100
#define PLL_IDSP_256MHZ_VAL	0x12000100

#if	defined(FIX_IDSP_108MHZ)
#define	EXPECT_PLL_IDSP_VAL	PLL_IDSP_108MHZ_VAL
#elif	defined(FIX_IDSP_148MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_148MHZ_VAL
#elif	defined(FIX_IDSP_202MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_202MHZ_VAL
#elif	defined(FIX_IDSP_216MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_216MHZ_VAL
#elif	defined(FIX_IDSP_256MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_256MHZ_VAL
#else

#if	defined(PWC_IDSP_108MHZ)
#define	EXPECT_PLL_IDSP_VAL	PLL_IDSP_108MHZ_VAL
#elif	defined(PWC_IDSP_148MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_148MHZ_VAL
#elif	defined(PWC_IDSP_202MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_202MHZ_VAL
#elif	defined(PWC_IDSP_216MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_216MHZ_VAL
#elif	defined(PWC_IDSP_256MHZ)
#define EXPECT_PLL_IDSP_VAL	PLL_IDSP_256MHZ_VAL
#else
#define EXPECT_PLL_IDSP_VAL	0xffffffff
#endif

#endif

/* SCALER_SD48_REG */
#define SD48_DUTY_CYCLE_CTRL	0x10000
#define SD48_PRIMARY_DIV	0x0ff00
#define SD48_INTEGER_DIV	0x000ff

/* SCALER_MS_REG */
#define MS_DUTY_CYCLE_CTRL	0x1000000
#define MS_DELAY_MUX		0x0c00000
#define MS_PRIMARY_DIV		0x01f0000
#define MS_INTEGER_DIV		0x000ffff

//#ifndef __ASSEMBLER__

/*
 * RCT/PLL functions
 */
//extern u32 get_apb_bus_freq_hz(void);
//extern u32 get_ahb_bus_freq_hz(void);
//extern u32 get_core_bus_freq_hz(void);

//#endif  /* __ASSEMBLER__ */

#endif
