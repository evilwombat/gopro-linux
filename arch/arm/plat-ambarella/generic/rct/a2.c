/**
 * system/src/peripheral/rct/a2.c
 *
 * History:
 *    2005/07/25 - [Charles Chiou] created file
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define VOUT_PLL_DEBUG
#ifdef VOUT_PLL_DEBUG
static int sp_retry = 0;
static int a2_retry = 0;
#endif

#define PLL_VIDEO		0
#define PLL_FREQ_HZ		REF_CLK_FREQ

static u32 so_clk_freq_hz 	= PLL_CLK_27MHZ;
static u32 spclk_freq_hz 	= PLL_CLK_27MHZ;
static u32 vo_clk_freq_hz 	= PLL_CLK_27MHZ;
static u32 vo_clk_scaling 	= 0;
static u32 vo_clk_src         	= VO_CLK_ONCHIP_PLL_27MHZ;

void rct_pll_init(void)
{

}

/**
 * Check the PLL lock status (A3) or wait for the PLL clock to lock (A2)
 */
void rct_alan_zhu_magic_loop(int clk_chk)
{
	mdelay(2);
}

u32 get_apb_bus_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	return get_core_bus_freq_hz() >> 1;
#endif
}

u32 get_ahb_bus_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	return get_core_bus_freq_hz();
#endif
}

u32 get_core_bus_freq_hz(void)
{
#if     defined(__FPGA__)
	return 48000000;
#else
	u32 n = readl(PLL_CORE_CTRL_REG);
	u32 x = (n & 0x00030000) >> 16;
	u32 y = (n & 0x0000c000) >> 8;
	u32 m = readl(SCALER_CORE_PRE_REG);

	n = (n & 0xff000000) >> 24;

	switch (x) {
	case 0x0: x = 8; break;
	case 0x1: x = 4; break;
	case 0x2: x = 2; break;
	case 0x3: x = 1; break;
	}

	switch (y) {
	case 0x00: y = 8; break;
	case 0x40: y = 4; break;
	case 0x80: y = 2; break;
	case 0xc0: y = 1; break;
	}

	return PLL_FREQ_HZ * n / 2 / x * y / m;
#endif
}

u32 get_dram_freq_hz(void)
{
	return get_core_bus_freq_hz();
}

u32 get_idsp_freq_hz(void)
{
	return get_core_bus_freq_hz();
}

u32 get_vout_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return vo_clk_freq_hz;
#endif
}

u32 get_vout2_freq_hz(void)
{
	return 0;
}

u32 get_adc_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readb(SCALER_ADC_REG);
#endif
}

void rct_set_adc_clk_src(int src)
{
}

u32 get_uart_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_UART_REG);
#endif
}

u32 get_ssi_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else

#if     defined(A2_METAL_REV_A1)
	/* A2_A1 chip uses apb bus clock frequency */
     	return (get_apb_bus_freq_hz() / readl(CG_HOST_REG));
#else
	return PLL_FREQ_HZ / readl(CG_SSI_REG);
#endif

#endif
}

u32 get_motor_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_MOTOR_REG);
#endif
}

u32 get_pwm_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return get_so_freq_hz() / readl(CG_PWM_REG);
#endif
}

u32 get_ir_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_IR_REG);
#endif
}

u32 get_host_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return (get_apb_bus_freq_hz() / readl(CG_HOST_REG));
#endif
}

u32 get_sd_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	u32 scaler = readl(SCALER_SD48_REG) & SD48_INTEGER_DIV;
	return (get_core_bus_freq_hz() * 2 / scaler);
#endif
}

/**
 * Get sensor clock out
 */
u32 get_so_freq_hz(void)
{
#if     defined(__FPGA__)
        return get_apb_bus_freq_hz();
#else
	return so_clk_freq_hz;
#endif
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
#if	!defined(__FPGA__)
	u32 val = readl(HOST_AHB_EARLY_TERMINATION_INFO);
	writel(HOST_AHB_EARLY_TERMINATION_INFO, 0x0);

	if (val == readl(HOST_AHB_EARLY_TERMINATION_INFO)) {
		*chip  = (val         & 0xff);
		*major = ((val >>  8) & 0xff);
		*minor = ((val >> 16) & 0xff);
	} else {
		writel(HOST_AHB_EARLY_TERMINATION_INFO, val);
		*chip  = 0x1;
		*major = 0x0;
		*minor = 0x0;
	}
#else
	*chip  = 0x0;
	*major = 0x0;
	*minor = 0x0;
#endif
}

u32 rct_boot_from(void)
{
	u32 rval = 0x0;
	u32 val = readl(SYS_CONFIG_REG);

	if ((val & SYS_CONFIG_FLASH_BOOT) != 0x0) {
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
	} else {
		/* USB boot */

		/* Force enabling flash access on USB boot */
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
	}

	if ((val & SYS_CONFIG_BOOT_BYPASS) != 0x0) {
		rval |= BOOT_FROM_BYPASS;
	}

	return rval;
}

int rct_is_cf_trueide(void)
{
	return 0;
}

int rct_is_eth_enabled(void)
{
	return (readl(SYS_CONFIG_REG) & SYS_CONFIG_ENET_SEL) != 0x0;
}

void rct_power_down(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | ANA_PWR_POWER_DOWN);
}

void rct_reset_chip(void)
{

#if	defined(__FPGA__)
	/* Do nothing... NOT supported! */
#else
	writel(SOFT_RESET_REG, 0x0);
	writel(SOFT_RESET_REG, 0x1);
#endif

}

void rct_reset_fio(void)
{
	register int c;

	writel(FIO_RESET_REG,
	       FIO_RESET_FIO_RST |
	       FIO_RESET_CF_RST  |
	       FIO_RESET_XD_RST  |
	       FIO_RESET_FLASH_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	mdelay(2);
}

void rct_reset_fio_only(void)
{
	register int c;

	writel(FIO_RESET_REG, FIO_RESET_FIO_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

void rct_reset_cf(void)
{
	register int c;

	writel(FIO_RESET_REG, FIO_RESET_CF_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

void rct_reset_flash(void)
{
	register int c;

	writel(FIO_RESET_REG, FIO_RESET_FLASH_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

void rct_reset_xd(void)
{
	register int c;

	writel(FIO_RESET_REG, FIO_RESET_XD_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

void rct_reset_dma(void)
{
	u32 val;
	register int c;

	val = readl(HOST_AHB_CLK_ENABLE_REG);
	val |= HOST_AHB_DMA_CHAN0_RST;
	writel(HOST_AHB_CLK_ENABLE_REG, val);

	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */

	val &= ~HOST_AHB_DMA_CHAN0_RST;
	writel(HOST_AHB_CLK_ENABLE_REG, val);

	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

void rct_set_uart_pll(void)
{
#if 0
#if	defined(PRK_UART_38400) || \
	defined(PRK_UART_57600) || \
	defined(PRK_UART_115200)
	/* Program UART RCT divider value to generate higher clock */
	writel(CG_UART_REG, 0x2);
#else
	/* Program UART RCT divider value to generate lower clock */
	writel(CG_UART_REG, 0x8);
#endif
#else
	writel(CG_UART_REG, 0x1);
#endif
}

void rct_set_sd_pll(u32 freq_hz)
{
	register u32 clk;

	/* Program the SD clock generator */
	clk = get_core_bus_freq_hz();

	if (clk >= 270000000) {
		/* Core == { 273MHz, 283MHz } */
		writeb(SCALER_SD48_REG, 0xc);
	} else if (clk >= 243000000) {
		/* Core == { 243MHz, 256Mhz } */
		writeb(SCALER_SD48_REG, 0xb);
	} else if (clk >= 230000000) {
		/* Core == 230MHz */
		writeb(SCALER_SD48_REG, 0xa);
	} else if (clk >= 216000000) {
		/* Core == 216MHz */
		writeb(SCALER_SD48_REG, 0x9);
	} else if (clk >= 182250000) {
		/* Core == 182.25MHz */
		writeb(SCALER_SD48_REG, 0x8);
	} else {
		/* Core below or equal to 135MHz */
		writeb(SCALER_SD48_REG, 0x6);
	}
}

void rct_set_ir_pll(void)
{
	writew(CG_IR_REG, 0x800);
}

void rct_set_ssi_pll(void)
{
#define SPI_CLK_DIV 	0x2

	writel(CG_SSI_REG, SPI_CLK_DIV);

#if	defined(A2_METAL_REV_A1)
	writel(CG_HOST_REG, get_apb_bus_freq_hz() / 13500000);
#endif
}

u32 get_ssi2_freq_hz(void)
{
	return 0;
}

void rct_set_ssi2_pll(void)
{
}

void rct_set_host_clk_freq_hz(u32 freq_hz)
{
	register u32 clk_div;

        clk_div  = get_apb_bus_freq_hz() / freq_hz;
	writel(CG_HOST_REG, clk_div);
}


/**
 * SO RCT runtime object.
 */
struct so_rct_obj_s {
	u32 	freq;
	u32 	sensor_ctrl;
	u32    	sensor_frac;
	u32    	scaler_pre;
	u32    	scaler_post;
};

/**
 * RCT register settings for SO with Fref = 27MHz
 */
static struct so_rct_obj_s G_so_rct[] = {
	/* Freq     		Ctrl 	    FRAC    PRE  POST */
        {PLL_CLK_13_5D1001MHZ, 	 0x0f019c02, 0xfbe9, 0x1, 0x10 },
	{PLL_CLK_13_5MHZ,  	 0x04003a0a, 0x0,    0x1, 0x8  },
	{PLL_CLK_22_5MHZ,  	 0x05003a0a, 0x0,    0x1, 0x6  },
 	{PLL_CLK_24D1001MHZ,  	 0x07017a02, 0xfaf4, 0x1, 0x9  },
	{PLL_CLK_24MHZ,  	 0x04013a0a, 0x0,    0x1, 0x9  },
	{PLL_CLK_24_3MHZ,  	 0x12019c0a, 0x0,    0x1, 0xa  },
        {PLL_CLK_25MHZ,  	 0x1903dc0a, 0x0,    0x1, 0x1b },
	{PLL_CLK_27D1001MHZ,  	 0x0f019902, 0xfbe9, 0x1, 0x8  },
        {PLL_CLK_27MHZ,  	 0x04003a0a, 0x0,    0x1, 0x4  },
        {PLL_CLK_27M1001MHZ,  	 0x1000e902, 0x0418, 0x1, 0x2  }, /* 27*1.001 MHz */
        {PLL_CLK_36D1001MHZ, 	 0x1702dc02, 0xf9dd, 0x1, 0x9  }, /* 36/1.001 MHz */
        {PLL_CLK_36MHZ,  	 0x1802dc0a, 0x0,    0x1, 0x9  },
        {PLL_CLK_37_125D1001MHZ, 0x15029902, 0xfa60, 0x1, 0x10 }, /* 74.25/1.001 MHz */
        {PLL_CLK_37_125MHZ,  	 0x16029c0a, 0x0,    0x1, 0x10 },
        {PLL_CLK_48D1001MHZ,  	 0x0f039c02, 0xfbe5, 0x1, 0x12 },
        {PLL_CLK_48MHZ,  	 0x04023a0a, 0x0,    0x1, 0x9  },
        {PLL_CLK_48_6MHZ,  	 0x12029c0a, 0x0,    0x1, 0xa  },
        {PLL_CLK_49_5D1001MHZ,   0x1d02fa02, 0x4dd5, 0x1, 0x8  }, /* 49.5/1.001 MHz */
        {PLL_CLK_49_5MHZ,  	 0x0b005a0a, 0x0,    0x1, 0x3  },
        {PLL_CLK_54MHZ,  	 0x04003a0a, 0x0,    0x1, 0x2  },
        {PLL_CLK_54M1001MHZ,  	 0x1000e902, 0x0418, 0x1, 0x1  }, /* 54*1.001 MHz */
        {PLL_CLK_60MHZ,		 0x1403d90a, 0x0,    0x1, 0x9  },
        {PLL_CLK_60M1001MHZ,	 0x1403d902, 0x51f,  0x1, 0x9  }, /* 60*1.001 MHz */
	{PLL_CLK_64D1001MHZ,  	 0x1503d902, 0x4ffd, 0x1, 0x9  }, /* 64/1.001 MHz */
	{PLL_CLK_64MHZ,    	 0x1503d902, 0x5556, 0x1, 0x9  },
        {PLL_CLK_74_25D1001MHZ,  0x15029902, 0xfa60, 0x1, 0x8  } , /* 74.25/1.001 MHz */
        {PLL_CLK_74_25MHZ,  	 0x16029c0a, 0x0,    0x1, 0x8  },
        {PLL_CLK_90MHZ,  	 0x14029c0a, 0x0,    0x1, 0x6  },
        {PLL_CLK_90_62D1001MHZ,  0x1403d902, 0x1de2, 0x1, 0x6  },
        {PLL_CLK_90_62MHZ, 	 0x1403d902, 0x2309, 0x1, 0x6  },
        {PLL_CLK_90_69D1001MHZ,  0x1403d902, 0x21f3, 0x1, 0x6  },
        {PLL_CLK_90_69MHZ,       0x1403d902, 0x271a, 0x1, 0x6  },
        {PLL_CLK_96D1001MHZ,	 0x1503d902, 0x4fe1, 0x1, 0x6  },  /* 96/1.001 MHz */
        {PLL_CLK_96MHZ,  	 0x1503d902, 0x5555, 0x1, 0x6  },
        {PLL_CLK_99_18D1001MHZ,  0x1603d902, 0x04a3, 0x1, 0x6  },
	{PLL_CLK_99_18MHZ,       0x1603d902, 0x0a48, 0x1, 0x6  },
	{PLL_CLK_108MHZ,  	 0x1003d90a, 0x0,    0x1, 0x4  },
        {PLL_CLK_148_5D1001MHZ,  0x15029902, 0xfa60, 0x1, 0x4  }, /* 148.5/1.001 MHz */
        {PLL_CLK_148_5MHZ, 	 0x16029c0a, 0x0,    0x1, 0x4  },
        {PLL_CLK_216MHZ,  	 0x1003d90a, 0x0,    0x1, 0x2  },
        {0x0, 	    		 0x04003a0a, 0x0,    0x1, 0x4  }  /* 27MHz as default */

};

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
};

/**
 * RCT register settings for VOUT pll with Fref = 27MHz
 */
static struct vout_rct_obj_s G_vout_rct[] = {

	/* Freq     		Ctrl 	    FRAC    PRE  POST  0S_RATIO*/
	{PLL_CLK_13_5D1001MHZ, 	0x0f019c02, 0xfbe9, 0x1, 0x10, 0x1},
	{PLL_CLK_13_5MHZ,  	0x10019c0a, 0x0,    0x1, 0x10, 0x1},
	{PLL_CLK_24_54MHZ,	0x0e019c02, 0x8ba2, 0x1, 0x8,  0x1},
	{PLL_CLK_27D1001MHZ,  	0x0f02d902, 0xfbe9, 0x1, 0x8,  0x1},
        {PLL_CLK_27MHZ,  	0x10019c0a, 0x0,    0x1, 0x8,  0x1},
	{PLL_CLK_26_9485MHZ,	0x0f00e902, 0xf832, 0x1, 0x2,  0x1}, /* - 1 LN at 60hz */
	{PLL_CLK_26_9568MHZ,	0x0f00e902, 0xf972, 0x1, 0x2,  0x1}, /* - 1 LN at 50hz */
	{PLL_CLK_27_0432MHZ,	0x1000e902, 0x068e, 0x1, 0x2,  0x1}, /* + 1 LN at 50hz */
	{PLL_CLK_27_0514MHZ,  	0x1000e902, 0x07cc, 0x1, 0x2,  0x1}, /* + 1 LN at 60hz */
        {PLL_CLK_27M1001MHZ,  	0x1000e902, 0x0418, 0x1, 0x2,  0x1}, /* 27*1.001 MHz */
        {PLL_CLK_54MHZ,  	0x1001d90a, 0x0,    0x1, 0x2,  0x2},
        {PLL_CLK_74_25D1001MHZ, 0x1503fa02, 0xfa60, 0x1, 0x8,  0x1}, /* 74.25/1.001 MHz */
        {PLL_CLK_74_25MHZ,  	0x1603fa0a, 0x0,    0x1, 0x8,  0x1},
        {PLL_CLK_90_62D1001MHZ, 0x1403d902, 0x1de2, 0x1, 0x6,  0x1},
        {PLL_CLK_90_62MHZ, 	0x1403d902, 0x2309, 0x1, 0x6,  0x1},
        {PLL_CLK_90_69D1001MHZ, 0x1403d902, 0x21f3, 0x1, 0x6,  0x1},
        {PLL_CLK_90_69MHZ,      0x1403d902, 0x271a, 0x1, 0x6,  0x1},
        {PLL_CLK_99_18D1001MHZ, 0x1603d902, 0x04a3, 0x1, 0x6,  0x1},
	{PLL_CLK_99_18MHZ,      0x1603d902, 0x0a48, 0x1, 0x6,  0x1},
        {PLL_CLK_108MHZ,  	0x1003d90a, 0x0,    0x1, 0x2,  0x2},
        {PLL_CLK_148_5D1001MHZ,	0x15009a02, 0xfa60, 0x1, 0x1,  0x1},
        {PLL_CLK_148_5MHZ, 	0x16009a0a, 0x0,    0x1, 0x1,  0x1}, /* Decrease VCO1 stress value */
        {0x0, 	    		0x10019c0a, 0x0,    0x1, 0x8,  0x1}  /* 27MHz as default */

};

static int so_freq_index 	= 0;
static int vout_freq_index 	= 0;

/**
 * Configure sensor clock out
 */
void rct_set_so_freq_hz(u32 freq_hz)
{

	int i;

	for (i = 0; ;i++) {
	        if ((G_so_rct[i].freq == 0) || (G_so_rct[i].freq == freq_hz))
			break;
	}
	so_freq_index = i;

	/* Configure sensor clock out. The reference clock = 27 MHz */
	writel(PLL_SENSOR_CTRL_REG, G_so_rct[i].sensor_ctrl);
        writel(PLL_SENSOR_FRAC_REG, G_so_rct[i].sensor_frac);
        writeb(SCALER_SENSOR_PRE_REG, G_so_rct[i].scaler_pre);
        writeb(SCALER_SENSOR_POST_REG, G_so_rct[i].scaler_post);

        rct_alan_zhu_magic_loop(0);
	so_clk_freq_hz = G_so_rct[i].freq;
}

/**
 *  Rescale the sensor clock frequency
 *  1 macrostep = 65536 microsteps
 */
void rct_rescale_so_pclk_freq_hz(u32 scale)
{
	int sign = scale & 0x80000000;
        u32 rescale, rescale_int, rescale_frac, reg;

	if (sign) {     /* Decrement */

  		rescale 	= (scale & 0x7fffffff);
  		rescale_int 	= rescale >> 16; 	/* # of macrosteps */
		rescale_frac	= rescale & 0xffff;     /* # of microsteps */

                reg 		= G_so_rct[so_freq_index].sensor_frac;
  	        if (reg >= rescale_frac) {
  		        rescale_frac    = reg - rescale_frac;
  		} else {
                        rescale_frac 	= reg + (65536 - rescale_frac);
                        rescale_int++;
		}
		writel(PLL_SENSOR_FRAC_REG, rescale_frac);

		reg 	= G_so_rct[so_freq_index].sensor_ctrl;
		K_ASSERT((reg >> 24) > rescale_int);
                reg    -= (rescale_int << 24);
                if (rescale_frac)
                	reg    &= ~(0x1 << 3);   	/* Disable DS */
		else
		        reg    |= (0x1 << 3);   	/* Enable DS */
                writel(PLL_SENSOR_CTRL_REG, reg);

	} else {        /* Increment */

                rescale 	= (scale & 0x7fffffff) +
				  G_so_rct[so_freq_index].sensor_frac;
		rescale_int 	= rescale >> 16; 	/* # of macrosteps */
		rescale_frac	= rescale & 0xffff;     /* # of microsteps */
                writel(PLL_SENSOR_FRAC_REG, rescale_frac);

	        reg 	= G_so_rct[so_freq_index].sensor_ctrl;
	        K_ASSERT((reg >> 24) + rescale_int < (0x1 << 7));
		reg	+= (rescale_int << 24);
	        if (rescale_frac)
                	reg    &= ~(0x1 << 3);   	/* Disable DS */
		else
		        reg    |= (0x1 << 3);   	/* Enable DS */
                writel(PLL_SENSOR_CTRL_REG, reg);
	}
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
	writeb(USE_CLK_SI_INPUT_MODE_REG, mode);
}

/**
 * Configure video clock source
 */
void rct_set_vout_clk_src(u32 clk_src)
{
	vo_clk_src = clk_src;
}

/* Configure video clock out with 27 MHz crystal clock */
static void rct_set_vout_pll_onchip(u32 freq_hz)
{
        int i;

	for (i = 0; ;i++) {
	        if ((G_vout_rct[i].freq == 0) ||
		    (G_vout_rct[i].freq == freq_hz))
			break;
	}
	vout_freq_index = i;

	/* Configure sensor clock out. The reference clock = 27 MHz */
        writel(SCALER_VIDEO_REG, G_vout_rct[i].scaler_pre);
        writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl);
	writel(PLL_VIDEO_FRAC_REG, G_vout_rct[i].video_frac);
        writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl | 0x10);
        writel(SCALER_VIDEO_POST_REG, G_vout_rct[i].scaler_post);
        writeb(SCALER_VIDEO_OS_RATION_REG, G_vout_rct[i].os_ratio);
	vo_clk_freq_hz = G_vout_rct[i].freq;
}

/**
 * VOUT RCT runtime object. video PLL using VIN pixel clock
 */
struct vout_rct_pclk_obj_s {
	u32 	fref;
	u32 	freq;
	u32 	video_ctrl;
	u32    	video_frac;
	u32    	scaler_pre;
	u32    	scaler_post;
	u32     os_ratio;
};

/**
 * RCT register settings for SO with Fref = 27MHz
 */
static struct vout_rct_pclk_obj_s G_vout_rct_pclk[] = {
	/* Fref			Freq
		Ctrl        	FRAC   	PRE  	POST  	0S_RATIO*/
        /* Fref = 13.5 MHz */
        {PLL_CLK_13_5MHZ, 	PLL_CLK_27D1001MHZ,
		0x0f019c02, 	0xfbe8,	0x1, 	0x04,  	0x1},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_27MHZ,
		0x10019c0a, 	0x0, 	0x1, 	0x04,  	0x1},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_27M1001MHZ,
		0x1000e902, 	0x0419,	0x1, 	0x01,  	0x1},

        /* Fref = 27 * 1.001 MHz */
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_27MHZ,
		0x0f00e902, 	0xfbe8, 0x1, 	0x02,  	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_27M1001MHZ,
		0x1000e90a, 	0x0,	0x1, 	0x02,  	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x05017a02, 	0x7d30, 0x1, 	0x02, 	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_74_25MHZ,
		0x05017a02, 	0x7e97, 0x1, 	0x02,  	0x1},
        {PLL_CLK_27M1001MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x05017a02, 	0x7d30, 0x1, 	0x01, 	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_148_5MHZ,
		0x05017a02, 	0x7e97, 0x1, 	0x01,  	0x1},

        /* Fref = 48 MHz */
	{PLL_CLK_48MHZ,  	PLL_CLK_27MHZ,
		0x0902dc0a, 	0x0,	0x1, 	0x04,  	0x2},
	{PLL_CLK_48MHZ,  	PLL_CLK_27M1001MHZ,
		0x0900fa02, 	0x024d,	0x1, 	0x01,  	0x2},
	{PLL_CLK_48MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x0c03d902, 	0x5cd6,	0x1, 	0x08,  	0x1},
	{PLL_CLK_48MHZ,  	PLL_CLK_74_25MHZ,
		0x0c03d902, 	0x6000,	0x1, 	0x08,  	0x1},
        {PLL_CLK_48MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x0c03d902, 	0x5cd6,	0x1, 	0x04,  	0x1},
	{PLL_CLK_48MHZ,  	PLL_CLK_148_5MHZ,
		0x0c03d902, 	0x6000,	0x1, 	0x04,  	0x1},

        /* Fref = 49.5 MHz */
        {PLL_CLK_49_5MHZ,  	PLL_CLK_27MHZ,
		0x06015a0a, 	0x0,	0x1, 	0x0b,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_27M1001MHZ,
		0x06015a02, 	0x0189,	0x1, 	0x0b,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x05005a02, 	0xfe77,	0x1, 	0x02,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_74_25MHZ,
		0x06005a0a, 	0x0,	0x1, 	0x02,  	0x1},
        {PLL_CLK_49_5MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x05005a02, 	0xfe77,	0x1, 	0x01,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_148_5MHZ,
		0x06005a0a, 	0x0,	0x1, 	0x01,  	0x1},

        /* Fref = 74.25/1.001 MHz */
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_27D1001MHZ,
		0x0803dc0a, 	0x0,	0x1, 	0x0b,  	0x2},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_27MHZ,
		0x0803d902, 	0x020c, 0x1, 	0x0b,  	0x2},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_27M1001MHZ,
        	0x0802cc02, 	0x0419,	0x1, 	0x0b,  	0x1},
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_74_25D1001MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x1},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_74_25MHZ,
		0x0803cc02, 	0x020c,	0x1, 	0x08,  	0x1},
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_148_5D1001MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x04,  	0x1},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_148_5MHZ,
		0x0803cc02, 	0x020c,	0x1, 	0x04,  	0x1},

        /* Fref = 74.25 MHz */
        {PLL_CLK_74_25MHZ, 	PLL_CLK_27MHZ,
		0x0803dc0a, 	0x0,	0x1, 	0x0b,  	0x2},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_27M1001MHZ,
		0x0802fa02, 	0x020c,	0x1, 	0x0b,  	0x1},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x1},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_74_25MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x1},
        {PLL_CLK_74_25MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x04,  	0x1},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_148_5MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x04,  	0x1},

        /* Fref = 90615840/1.001 MHz */
        {PLL_CLK_90_62D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc5ab,	0x1, 	0x04,  	0x2},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc6e4,	0x1, 	0x02,  	0x2},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8e1e,	0x1, 	0x02,  	0x1},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8fcc, 0x1, 	0x02,  	0x1},
        {PLL_CLK_90_62D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8e1e,	0x1, 	0x01,  	0x1},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8fcc, 0x1, 	0x01,  	0x1},

        /* Fref = 90615840 MHz */
        {PLL_CLK_90_62MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc473,	0x1, 	0x04,  	0x2},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc5ab,	0x1, 	0x02,  	0x2},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8c71,	0x1, 	0x02,  	0x1},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8e1e, 0x1, 	0x02,  	0x1},
        {PLL_CLK_90_62MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8c71,	0x1, 	0x01,  	0x1},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8e1e, 0x1, 	0x01,  	0x1},

        /* Fref = 90687360/1.001 MHz */
        {PLL_CLK_90_69D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc4b5,	0x1, 	0x04,  	0x2},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc5ed,	0x1, 	0x02,  	0x2},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8ccb,	0x1, 	0x02,  	0x1},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8e78, 0x1, 	0x02,  	0x1},
        {PLL_CLK_90_69D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8ccb,	0x1, 	0x01,  	0x1},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8e78, 0x1, 	0x01,  	0x1},

        /* Fref = 90687360 MHz */
        {PLL_CLK_90_69MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc37d,	0x1, 	0x04,  	0x2},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc4b5,	0x1, 	0x02,  	0x2},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8b1e,	0x1, 	0x02,  	0x1},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8ccb, 0x1, 	0x02,  	0x1},
        {PLL_CLK_90_69MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8b1e,	0x1, 	0x01,  	0x1},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8ccb, 0x1, 	0x01,  	0x1},

        /* Fref = 95992800/1.001 MHz */
        {PLL_CLK_95_993D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x813d,	0x1, 	0x04,  	0x2},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0x8264,	0x1, 	0x02,  	0x2},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x301e,	0x1, 	0x02,  	0x1},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x31b4, 0x1, 	0x02,  	0x1},
        {PLL_CLK_95_993D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x301e,	0x1, 	0x01,  	0x1},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x31b4, 0x1, 	0x01,  	0x1},

	/* Fref = 96/1.001 MHz */
        {PLL_CLK_96D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x8127,	0x1, 	0x04,  	0x2},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0x824e,	0x1, 	0x02,  	0x2},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601fa02, 	0x3000,	0x1, 	0x02,  	0x1},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x3196, 0x1, 	0x02,  	0x1},
        {PLL_CLK_96D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x3000,	0x1, 	0x01,  	0x1},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x3196, 0x1, 	0x01,  	0x1},

	/* Fref = 96 MHz */
        {PLL_CLK_96MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x8000,	0x1, 	0x04,  	0x2},
	{PLL_CLK_96MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0x8126,	0x1, 	0x02,  	0x2},
	{PLL_CLK_96MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601fa02, 	0x2e6b,	0x1, 	0x02,  	0x1},
	{PLL_CLK_96MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x3000, 0x1, 	0x02,  	0x1},
        {PLL_CLK_96MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601fa02, 	0x2e6b,	0x1, 	0x01,  	0x1},
	{PLL_CLK_96MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x3000, 0x1, 	0x01,  	0x1},

        /* Fref = 99/1.001 MHz */
        {PLL_CLK_99D1001MHZ, 	PLL_CLK_27MHZ,
		0x0602da02, 	0x0189,	0x1, 	0x0b,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0602da02, 	0x0312,	0x1, 	0x0b,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601da0a, 	0x0,	0x1, 	0x02,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601da02, 	0x0189, 0x1, 	0x02,  	0x1},
        {PLL_CLK_99D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601da0a, 	0x0,	0x1, 	0x01,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601da02, 	0x0189, 0x1, 	0x01,  	0x1},

        /* Fref = 99 MHz */
        {PLL_CLK_99MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x5c2d, 0x1, 	0x04,  	0x2},
	{PLL_CLK_99MHZ, 	PLL_CLK_27M1001MHZ,
		0x0602da02, 	0x0189,	0x1, 	0x0b,  	0x1},
	{PLL_CLK_99MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0501de02, 	0xfe77,	0x1, 	0x02,  	0x1},
	{PLL_CLK_99MHZ, 	PLL_CLK_74_25MHZ,
		0x0601da0a, 	0x0,	0x1, 	0x02,  	0x1},
        {PLL_CLK_99MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0501de02, 	0xfe77,	0x1, 	0x01,  	0x1},
	{PLL_CLK_99MHZ, 	PLL_CLK_148_5MHZ,
		0x0601da0a, 	0x0,	0x1, 	0x01,  	0x1},

        /* Fref = 99.18 /1.001 MHz */
        {PLL_CLK_99_18D1001MHZ,	PLL_CLK_27MHZ,
		0x0402d902, 	0x5c2d,	0x1, 	0x04,  	0x2},
	{PLL_CLK_99_18D1001MHZ,	PLL_CLK_27M1001MHZ,
		0x0401da02, 	0x5d4a,	0x1, 	0x02,  	0x2},
	{PLL_CLK_99_18D1001MHZ,	PLL_CLK_74_25D1001MHZ,
		0x0502de02, 	0xfd34,	0x1, 	0x02,  	0x2},
	{PLL_CLK_99_18D1001MHZ,	PLL_CLK_74_25MHZ,
		0x0502da02, 	0xfebd, 0x1, 	0x02,  	0x2},
        {PLL_CLK_99_18D1001MHZ,	PLL_CLK_148_5D1001MHZ,
		0x0502da02, 	0xfd34,	0x1, 	0x02,  	0x1},
	{PLL_CLK_99_18D1001MHZ,	PLL_CLK_148_5MHZ,
		0x0502da02, 	0xfebd, 0x1, 	0x02,  	0x1},

	/* Fref = 99.18 MHz */
        {PLL_CLK_99_18MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x5b0f,	0x1, 	0x04,  	0x2},
	{PLL_CLK_99_18MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401da02, 	0x5c2c,	0x1, 	0x02,  	0x2},
	{PLL_CLK_99_18MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0501de02, 	0xfbac,	0x1, 	0x02,  	0x1},
	{PLL_CLK_99_18MHZ, 	PLL_CLK_74_25MHZ,
		0x0501da02, 	0xfd34, 0x1, 	0x02,  	0x1},
        {PLL_CLK_99_18MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0501de02, 	0xfbab,	0x1, 	0x01,  	0x1},
	{PLL_CLK_99_18MHZ, 	PLL_CLK_148_5MHZ,
		0x0501da02, 	0xfd34, 0x1, 	0x01,  	0x1},

        /* Fref = 108 MHz, TBD */
        {PLL_CLK_108MHZ, 	PLL_CLK_27MHZ,
		0x0803dc0a, 	0x0,	0x1, 	0x0b,  	0x4},
	{PLL_CLK_108MHZ, 	PLL_CLK_27M1001MHZ,
		0x0802fa02, 	0x020c,	0x1, 	0x0b,  	0x2},
	{PLL_CLK_108MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x2},
	{PLL_CLK_108MHZ, 	PLL_CLK_74_25MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x2},
        {PLL_CLK_108MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x1},
	{PLL_CLK_108MHZ, 	PLL_CLK_148_5MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x1},


        /* Fref = 148.5/1.001 MHz */
        {PLL_CLK_148_5D1001MHZ, PLL_CLK_27D1001MHZ,
		0x0803dc0a, 	0x0,	0x1, 	0x0b,  	0x4},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_27MHZ,
		0x0803d902, 	0x020c, 0x1, 	0x0b,  	0x4},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_27M1001MHZ,
        	0x0802cc02, 	0x0419,	0x1, 	0x0b,  	0x2},
        {PLL_CLK_148_5D1001MHZ, PLL_CLK_74_25D1001MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x2},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_74_25MHZ,
		0x0803cc02, 	0x020c,	0x1, 	0x08,  	0x2},
        {PLL_CLK_148_5D1001MHZ, PLL_CLK_148_5D1001MHZ,
		0x0403cc0a, 	0x0,	0x1, 	0x02,  	0x2},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_148_5MHZ,
		0x0403cc02, 	0x0106,	0x1, 	0x02,  	0x2},

        /* Fref = 148.5 MHz */
        {PLL_CLK_148_5MHZ, 	PLL_CLK_27MHZ,
		0x0803dc0a, 	0x0,	0x1, 	0x0b,  	0x4},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_27M1001MHZ,
		0x0802fa02, 	0x020c,	0x1, 	0x0b,  	0x2},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x2},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_74_25MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x2},
        {PLL_CLK_148_5MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x1},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_148_5MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x1},

        /* Fref = 216 MHz, TBD*/
        {PLL_CLK_216MHZ, 	PLL_CLK_27MHZ,
		0x0803dc0a, 	0x0,	0x1, 	0x0b,  	0x4},
	{PLL_CLK_216MHZ, 	PLL_CLK_27M1001MHZ,
		0x0802fa02, 	0x020c,	0x1, 	0x0b,  	0x2},
	{PLL_CLK_216MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x2},
	{PLL_CLK_216MHZ, 	PLL_CLK_74_25MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x2},
        {PLL_CLK_216MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1, 	0x08,  	0x1},
	{PLL_CLK_216MHZ, 	PLL_CLK_148_5MHZ,
		0x0803cc0a, 	0x0,	0x1, 	0x08,  	0x1},

        /* 27MHz as default */
        {0x0, 	    		0,
		0x0,   		0x0,	0x0, 	0x0,  	0x0}

};

/* Configure video clock out with pixel clock or clock_si */
static void rct_set_vout_pll_spclk(u32 freq_hz)
{
	int i;

	if (spclk_freq_hz == PLL_CLK_27MHZ) {
	        rct_set_vout_pll_onchip(freq_hz);
	} else {
		for (i = 0; ;i++) {
		        if ((G_vout_rct_pclk[i].fref == 0) ||
			    ((G_vout_rct_pclk[i].fref == spclk_freq_hz) &&
			     (G_vout_rct_pclk[i].freq == freq_hz)) )
				break;
		}

		vout_freq_index = i;

		if (G_vout_rct_pclk[i].fref != 0) {
	        	writel(SCALER_VIDEO_REG, G_vout_rct_pclk[i].scaler_pre);
                        writel(PLL_VIDEO_CTRL_REG,
				       	G_vout_rct_pclk[i].video_ctrl);
	        	writel(PLL_VIDEO_FRAC_REG,
				       	G_vout_rct_pclk[i].video_frac);
                        writel(PLL_VIDEO_CTRL_REG,
					G_vout_rct_pclk[i].video_ctrl | 0x10);
			writeb(SCALER_VIDEO_POST_REG,
					       G_vout_rct_pclk[i].scaler_post);
			writeb(SCALER_VIDEO_OS_RATION_REG,
					       G_vout_rct_pclk[i].os_ratio);
			vo_clk_freq_hz = G_vout_rct_pclk[i].freq;
	        } else {
	                /* Fref = 27MHz as default */
	                rct_set_vout_pll_onchip(freq_hz);
		}
	}
}

static int rct_rescale_vout_pll(u32 scale)
{
        int sign = scale & 0x80000000;
        u32 rescale, rescale_int, rescale_frac;
	u32 frac_reg, ctrl_reg;
	u32 cur_clk_src = 0, video_frac_reg = 0, video_ctrl_reg = 0;

        cur_clk_src 	= vo_clk_src;
	video_frac_reg	= PLL_VIDEO_FRAC_REG;
	video_ctrl_reg 	= PLL_VIDEO_CTRL_REG;

        if  (cur_clk_src == VO_CLK_EXTERNAL) {
 		printk("Not supported for the external cloc");
		return -1;
	}

        if ((cur_clk_src == VO_CLK_ONCHIP_PLL_CLK_SI) ||
            (cur_clk_src == VO_CLK_ONCHIP_PLL_SP_CLK) ) {
              	frac_reg = G_vout_rct_pclk[vout_freq_index].video_frac;
               	ctrl_reg = G_vout_rct_pclk[vout_freq_index].video_ctrl;
	} else {
	       	frac_reg = G_vout_rct[vout_freq_index].video_frac;
               	ctrl_reg = G_vout_rct[vout_freq_index].video_ctrl;
	}

	if (sign) {     /* Decrement */
  		rescale 	= (scale & 0x7fffffff);
  		rescale_int 	= rescale >> 16; 	/* # of macrosteps */
		rescale_frac	= rescale & 0xffff;     /* # of microsteps */

  	        if (frac_reg >= rescale_frac) {
  		        rescale_frac    = frac_reg - rescale_frac;
  		} else {
                        rescale_frac 	= frac_reg + (65536 - rescale_frac);
                        rescale_int++;
		}
		writel(video_frac_reg, rescale_frac);

		K_ASSERT((ctrl_reg >> 24) > rescale_int);
                ctrl_reg    -= (rescale_int << 24);
                if (rescale_frac)
                	ctrl_reg    &= ~(0x1 << 3);   	/* Disable DS */
		else
		        ctrl_reg    |= (0x1 << 3);   	/* Enable DS */
                writel(video_ctrl_reg, ctrl_reg);

	} else {        /* Increment */
                rescale 	= (scale & 0x7fffffff) + frac_reg;
		rescale_int 	= rescale >> 16; 	/* # of macrosteps */
		rescale_frac	= rescale & 0xffff;     /* # of microsteps */
                writel(video_frac_reg, rescale_frac);

	        K_ASSERT((ctrl_reg >> 24) + rescale_int < (0x1 << 7));
		ctrl_reg	+= (rescale_int << 24);
	        if (rescale_frac)
                	ctrl_reg    &= ~(0x1 << 3);   	/* Disable DS */
		else
		        ctrl_reg    |= (0x1 << 3);   	/* Enable DS */
                writel(video_ctrl_reg, ctrl_reg);
	}

	return 0;
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
 */
void rct_rescale_vout_clk_freq_hz(u32 scale)
{
	vo_clk_scaling = scale;
        rct_rescale_vout_pll(scale);
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
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
}

static void rct_config_vout_pll_reg(u32 freq_hz)
{

	u32 ctrl;
        int retry = 10;
	u32 use_ext_clk 	= USE_EXTERNAL_VD_CLK_REG;
	u32 clk_ref_ext 	= CLK_REF_VIDEO_EXTERNAL_REG;
	u32 use_clk_si		= USE_CLK_SI_4_VO_REG;
	u32 use_clk_si_inp	= USE_CLK_SI_INPUT_MODE_REG;
	u32 pll_ctrl		= PLL_VIDEO_CTRL_REG;
	u32 clk_src		= vo_clk_src;
	u32 vd_ctrl_reg		= VOUT_CTL_CONTROL_REG;

        switch (clk_src) {
	case VO_CLK_ONCHIP_PLL_CLK_SI:
		do {
                	rct_set_vout_pll_spclk(freq_hz);

                	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x1);
	        	writeb(use_clk_si, 0x1);

	        	/* SI clock input mode */
	        	writeb(use_clk_si_inp, 1);
	        	ctrl = readl(pll_ctrl) & ~0x10;
			writel(pll_ctrl, ctrl);
			rct_alan_zhu_magic_loop(0);

                        if (readl(vd_ctrl_reg) != 0)
                                break;
			else {
			        retry--;
			}
//#ifdef VOUT_PLL_DEBUG
                	printk("Retry(%d)", retry);
//#endif
  		} while (retry);

                break;
        case VO_CLK_ONCHIP_PLL_SP_CLK:
                do {
	        	rct_set_vout_pll_spclk(freq_hz);
	        	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x1);
	        	writeb(use_clk_si, 0x0);
	        	writeb(use_clk_si_inp, 0); /* SI clock input mode */
	        	ctrl = readl(pll_ctrl) & ~0x10;
	        	writel(pll_ctrl, ctrl);
			rct_alan_zhu_magic_loop(0);

                        if (readl(vd_ctrl_reg) != 0)
                                break;
			else {
			        retry--;
#ifdef VOUT_PLL_DEBUG
			        sp_retry++;
			        printk("SPCLK : %d Retry(%d)", sp_retry, retry);
#endif
			}
                } while (retry);

	        break;
	case VO_CLK_EXTERNAL:
	        do {
	        	ctrl = readl(pll_ctrl);
			writel(pll_ctrl, ctrl | 0x10);
			writeb(use_ext_clk, 0x1);
			writel(pll_ctrl, ctrl);
			rct_alan_zhu_magic_loop(0);

                        if (readl(vd_ctrl_reg) != 0)
                                break;
			else {
			        retry--;
			}
#ifdef VOUT_PLL_DEBUG
                	printk("Retry(%d)", retry);
#endif

		} while (retry);

	        break;
	default:        /* VO_CLK_PLL_27MHZ */
		vo_clk_src = VO_CLK_ONCHIP_PLL_27MHZ;

		do {
			rct_set_vout_pll_onchip(freq_hz);

                	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x0);
	        	writeb(use_clk_si, 0x0);
	        	writeb(use_clk_si_inp, 0); /* SI clock output mode */
	        	ctrl = readl(pll_ctrl) & ~0x10;
	        	writel(pll_ctrl, ctrl);
			rct_alan_zhu_magic_loop(0);

                        if (readl(vd_ctrl_reg) != 0)
                                break;
			else {
			        retry--;
#ifdef VOUT_PLL_DEBUG
			        a2_retry++;
			        printk("Onchip : %d Retry(%d)", a2_retry, retry);
#endif
			}
		} while (retry);
	};

#ifdef VOUT_PLL_DEBUG
  		if (retry == 0) {
                      	printk("Retry failed!!!!");
			K_ASSERT(0);
		}
#endif
}

/**
 * Configure video clock out
 */
void rct_set_vout2_freq_hz(u32 freq_hz)
{
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
 */
void rct_rescale_vout2_clk_freq_hz(u32 scale)
{
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
 */
u32 get_vout2_clk_rescale_value(void)
{
	return 0;
}

/**
 * Configure video clock out
 */
void rct_set_vout_freq_hz(u32 freq_hz)
{
	rct_config_vout_pll_reg(freq_hz);
}

/**
 * Configure stepping motor clock frequency
 */
void rct_set_motor_freq_hz(u32 freq_hz)
{
        register u32 clk;

        clk     = PLL_FREQ_HZ / freq_hz;
        writel(CG_MOTOR_REG, clk);
}

/**
 * Configure stepping motor clock frequency
 */
void rct_set_pwm_freq_hz(u32 freq_hz)
{
        register u32 clk;

        clk     = get_so_freq_hz() / freq_hz;
        writel(CG_PWM_REG, (clk));
}

void rct_set_usb_ana_on(void)
{
	writel(PLL_USB_CTRL_REG, 0x1003FA0A); /* better jitter */
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | 6);
}

void rct_suspend_usb(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x06);
}

/*
 * Used by low-level usb driver initialization
 */
void rct_set_usb_clk(void)
{
#if defined(CONFIG_BSP_JIG) || defined(CONFIG_SENSOR_ALTA2462L2)
	rct_set_usb_ext_clk();
#else
	rct_ena_usb_int_clk();
#endif
}

void rct_set_usb_ext_clk(void)
{
}

void rct_ena_usb_int_clk(void)
{
}

void rct_set_usb_debounce(void)
{
}

void rct_turn_off_usb_pll(void)
{
	writel(ANA_PWR_REG, 0);
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
	udelay(150);
}

void rct_set_adc_clk_freq_hz(u32 freq_hz)
{
	register u32 clk_div;

        clk_div     = PLL_FREQ_HZ / freq_hz;
	writel(SCALER_ADC_REG, clk_div);
}

void rct_set_vin_lvds_pad(int mode)
{
}

