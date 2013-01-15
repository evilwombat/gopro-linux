/**
 * system/src/peripheral/rct/a5.c
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

#define ENABLE_DEBUG_MSG_RCT
#ifdef ENABLE_DEBUG_MSG_RCT
#define DEBUG_MSG	printk
#else
#define DEBUG_MSG(...)
#endif

//#define VOUT_PLL_DEBUG
#ifdef VOUT_PLL_DEBUG
static int sp_retry = 0;
static int a5_retry = 0;
#endif

#define PLL_VIDEO               0
#define PLL_VIDEO2              1
#define PLL_SO			3

#define PLL_FREQ_HZ		REF_CLK_FREQ

static u32 so_clk_freq_hz 	= PLL_CLK_27MHZ;
static u32 spclk_freq_hz 	= PLL_CLK_27MHZ;
static u32 vo_clk_freq_hz 	= PLL_CLK_27MHZ;
static u32 vo2_clk_freq_hz 	= PLL_CLK_27MHZ;
static u32 vo_clk_scaling 	= 0;
static u32 vo2_clk_scaling 	= 0;
static u32 vo_clk_src         	= VO_CLK_ONCHIP_PLL_27MHZ;
static u32 vo2_clk_src        	= VO_CLK_ONCHIP_PLL_27MHZ;
static u32 hdmi_clk_src         = HDMI_CLK_ONCHIP_PLL;

static int rct_rescale_pll(int id, u32 scale);

#define do_some_delay()		dly_tsk(1);

void rct_pll_init(void)
{
	u32 val;

	/* Initialized PLL setting */
	/* PLL CTRL3 */
	amba_outl(0x70170104, 0x00068300);
	amba_outl(0x7017010c, 0x00068300);
	amba_outl(0x70170114, 0x00068300);
	amba_outl(0x70170120, 0x00068300);
/*	amba_outl(0x7017012c, 0x00068300); */
	amba_outl(0x70170134, 0x00068300);
	amba_outl(0x70170140, 0x00068300);
	amba_outl(0x7017014c, 0x00068300);
	amba_outl(0x70170154, 0x00068300);
	amba_outl(0x701701bc, 0x00068308);

	/* PLL CTRL2 */
	amba_outl(0x70170100, 0x3f770000);
	amba_outl(0x70170108, 0x3f770000);
	amba_outl(0x70170110, 0x3f770000);
	amba_outl(0x7017011c, 0x3f770000);
/*	amba_outl(0x70170124, 0x3f770000); */
/*	amba_outl(0x70170130, 0x3f770000); */
/*	amba_outl(0x7017013c, 0x3f770000); */
	amba_outl(0x70170144, 0x3f770000);
	amba_outl(0x70170150, 0x3f770000);
	amba_outl(0x701701b8, 0x3f1f0000);

	/* IDSP pll ctrl reg */

	/* If the constant is 0xffffffff, then we use the POWER_ON_CONFIG
	   default */
	if ((EXPECT_PLL_IDSP_VAL != 0xffffffff) &&
	/* Check if the PLL_IDSP_CTR register value equals to the constant
	   that we expect. If not, then we program it with the desired
	   value and trigger a chip reset */
	    ((amba_inl(0x701700e4) & 0xfffffffe) !=
	     (EXPECT_PLL_IDSP_VAL & 0xfffffffe))) {
		amba_outl(0x701700e4, (EXPECT_PLL_IDSP_VAL & 0xfffffffe));
		do_some_delay();
		amba_outl(0x701700e4, (EXPECT_PLL_IDSP_VAL | 0x00000001));
	}

	/*
	 * hacked video PLL
	 */
	/* video ctrl2 */
	amba_outl(0x70170130, 0x3f770000);

	/* scalers */
	amba_outl(0x7017001c, 0x00010002);
	amba_outl(0x7017001c, 0x00010001);

	/* post-scalers, write a value different than default(0x10001),
	   then write back */
	amba_outl(0x701700a0, 0x10002);
	amba_outl(0x701700a0, 0x10001);

	/* pll ctrl reg (int, sdiv, sout) */
	amba_outl(0x70170014, 0x0a130104);
	do_some_delay();
	amba_outl(0x70170014, 0x0a130105);
	do_some_delay();

	/* Check video clock is locked or not */
	val = amba_inl(0x60008800);

	/*
	 * hacked video  (2) PLL
	 */
	/* video ctrl2 */
	amba_outl(0x7017013c, 0x3f770000);

	/* scalers */
	amba_outl(0x701700c8, 0x00010002);
	amba_outl(0x701700c8, 0x00010001);

	/* pll ctrl reg (int, sdiv, sout) */
	amba_outl(0x701700c0, 0x0a130104);
	do_some_delay();
	amba_outl(0x701700c0, 0x0a130105);
	do_some_delay();

	/* Check video clock is locked or not */
	val = amba_inl(0x60011800);

	/* gclk_hdmi = 10 * gclk_vo when 0x7017008 bit-0 = 0*/
        writel(PLL_HDMI_CTRL_REG,    0x09011100);
        writel(PLL_HDMI_FRAC_REG,    0x0);
	writel(SCALER_HDMI_POST_REG, 0x0010001);
	writel(PLL_HDMI_CTRL_REG,    0x09011101);
	writel(SCALER_HDMI_PRE_REG,  0x0010001);

	/* gclk_vin init */
	writel(SCALER_VIN_PRE_REG,  0x0010004);
	writel(SCALER_VIN_POST_REG, 0x0010002);
        //writel(PLL_VIN_CTRL_REG,    0x17053100);
        //writel(PLL_VIN_FRAC_REG,    0x0);
       	//writel(PLL_VIN_CTRL_REG,    0x17053101);

	/*
	 * Settings for 12.288MHz audio clock
	 */
 	/* prescalers */
	amba_outl(0x70170060, 0x00010002);
	amba_outl(0x70170060, 0x00010001);

	/* ctrl2 & ctrl3 */
	amba_outl(0x70170124, 0x3f770000);
	amba_outl(0x7017012c, 0x00069300);

	/* pll ctrl reg (int, sdiv & sout) */
	amba_outl(0x70170054, 0x13031118);

	/* frac reg */
	amba_outl(0x70170058, 0x65f1e43);
	do_some_delay();

	amba_outl(0x70170054, 0x13031108);
	do_some_delay();

	/* postscalers */
	amba_outl(0x7017005c, 0x00010011);//fake
	do_some_delay();
	amba_outl(0x7017005c, 0x00010016);
	do_some_delay();

	amba_outl(0x70170054, 0x13031109);

	amba_outl(0x70180094, 0x80000000);  /* Reset AORC */

	/*
	 * Miscellaneous...
	 */
	amba_outl(0x7017004c, 0x10003);  /* SO PLL prescaler */
  	amba_outl(0x7017004c, 0x10002);
	amba_outl(0x70170030, 0x10003);
  	amba_outl(0x70170030, 0x10002);
  	amba_outl(0x70170024, 0x0b031104);
  	do_some_delay();
  	amba_outl(0x70170024, 0x0b031105);

	/* Power control */
	amba_outl(0x70170098, 0x20);      	/* Sensor PAD */

	amba_outl(0x7011801c, 0xff);

	/* Init SD pll */
	writel(SCALER_SD48_REG,
	       (readl(SCALER_SD48_REG) & 0xffff0000) | (0x1));

	/* Margin registers to have correct speed selection of vDSP' SRAM. */
	amba_outl(0x70150108, 0x01000000);
	amba_outl(0x70160100, 0x01000000);
}

/**
 * Check the PLL lock status (A3) or wait for the PLL clock to lock (A2)
 */
void rct_alan_zhu_magic_loop(int clk_chk)
{
	u16 reg = readw(PLL_LOCK_REG) & clk_chk;
	register u32 loop_cnt = (get_core_bus_freq_hz() / 500);

	while ((reg == 0) && loop_cnt) {
                reg = readw(PLL_LOCK_REG) & clk_chk;
                loop_cnt--;
	}

	/* Check the timeout */
	//K_ASSERT(loop_cnt);
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

#if 0	/* A5-A0 */
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);
	u32 c = readl(PLL_CORE_CTRL_REG);
	u32 s = readl(SCALER_CORE_PRE_REG);
#else	/* A5-A1 */
	/* 0, 3 and 6 bit are reserved */
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);
	/* 12 ~ 19 bit are reserved */
	u32 c = (readl(PLL_CORE_CTRL_REG) & 0xfff00fff);
#endif	/* A5-Ax */

	u32 intprog;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		return 0;
	}

	intprog += 1 + z;

	/*
	 * PLL_FREQ_HZ * intprog * sdiv /sout / 2
	 * sdiv = 1
	 * sout = 2
	 */
	return PLL_FREQ_HZ * intprog / 2 / 2;
#endif
}

static u32 get_idsp_dram_pll_freq(u32 z, u32 f, u32 c, u32 s)
{
	u32 intprog, sout, sdiv, p, n;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);
	p = SCALER_PRE_P(s);
	n = SCALER_PRE_N(s);

	if (p != 1 && p != 2 && p != 3 && p != 5 && p != 7 &&
	    p != 9 && p != 11 && p != 13 && p != 17 && p != 19 &&
	    p != 23 && p != 29 && p != 32)
		p = 2;

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		return 0;
	}

	intprog += 1 + z;
	sdiv++;
	sout++;

	return PLL_FREQ_HZ * intprog * sdiv / sout / p / n / 2;
}

u32 get_dram_freq_hz(void)
{
	u32 z = (readl(PLL_DDR_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_DDR_FRAC_REG);
	u32 c = readl(PLL_DDR_CTRL_REG);
	u32 s = readl(SCALER_CORE_PRE_REG);

	return get_idsp_dram_pll_freq(z, f, c, s);
}

u32 get_idsp_freq_hz(void)
{
	u32 z = (readl(PLL_IDSP_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_IDSP_FRAC_REG);
	u32 c = readl(PLL_IDSP_CTRL_REG);
	u32 s = readl(SCALER_CORE_PRE_REG);

	return get_idsp_dram_pll_freq(z, f, c, s);
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
	return vo2_clk_freq_hz;
}


/**
 * FIXME
 */
u32 get_adc_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	if ((readl(ADC16_CTRL_REG) & (0x1 << 22)) == 0) {
		return PLL_FREQ_HZ / readl(SCALER_ADC_REG);
	} else {
		return get_audio_freq_hz();
	}
#endif
}

void rct_set_adc_clk_src(int src)
{
	writel(ADC16_CTRL_REG, (src & (0x01 << 22)));
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
	return (get_apb_bus_freq_hz() / (readl(CG_SSI_REG)));
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

void rct_set_ir_pll(void)
{
	writew(CG_IR_REG, 0x800);
}

void rct_set_ssi_pll(void)
{
	writel(CG_SSI_REG, get_apb_bus_freq_hz() / 13500000);
}

u32 get_ssi2_freq_hz(void)
{
	return (get_apb_bus_freq_hz() / (readl(CG_SSI2_REG) & 0xffffff));
}

void rct_set_ssi2_pll(void)
{
	/* 25 bits, bit-24 = 1 enable ssi2_clk */
	writel(CG_SSI2_REG,
	       ((0x1 << 24)  | (get_apb_bus_freq_hz() / 13500000)));
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
	u32	frac_100khz; 	/* Frac PLL change for 100KHz */
};

/**
 * RCT register settings for SO with Fref = 27MHz
 */
static struct so_rct_obj_s G_so_rct[] = {
	/* Freq     		 Ctrl 	     FRAC    	 PRE
						POST 	FRAC_100KHZ */
        {PLL_CLK_27MHZ,  	 0x0b131100, 0x0,        0x10001, 0x10006},
	{PLL_CLK_10D1001MHZ, 	 0x0e121108, 0xfae296e3, 0x10001, 0x1001b},
	{PLL_CLK_10MHZ, 	 0x09111100, 0x00000000, 0x10001, 0x1001b},
        {PLL_CLK_13_5D1001MHZ, 	 0x0f131108, 0xfbe878b8, 0x10001, 0x10010},
	{PLL_CLK_13_5MHZ,  	 0x0f131100, 0x0,    	 0x10001, 0x10010},
	{PLL_CLK_22_5MHZ,  	 0x09153100, 0x0, 	 0x10001, 0x10008},
 	{PLL_CLK_24D1001MHZ,  	 0x0d131108, 0x35406b4d, 0x10001, 0x10008},
	{PLL_CLK_24MHZ,  	 0x0f111100, 0x0, 	 0x10001, 0x10012},
	{PLL_CLK_24M1001MHZ, 	 0x0f111108, 0x04189375, 0x10001, 0x10012},
	{PLL_CLK_24_3MHZ,  	 0x0d131108, 0x66666667, 0x10001, 0x10008},
        {PLL_CLK_25MHZ,  	 0x0e131108, 0xd097b425, 0x10001, 0x10008},
	{PLL_CLK_27D1001MHZ,  	 0x0f131108, 0xfbe878b6, 0x10001, 0x10008},
        {PLL_CLK_27M1001MHZ,  	 0x0f131108, 0x04189375, 0x10001, 0x10008}, /* 27*1.001 MHz */
        {PLL_CLK_30MHZ,		 0x09113108, 0x0, 	 0x10001, 0x10012},
        {PLL_CLK_36D1001MHZ, 	 0x14131108, 0x4fe0a0f3, 0x10001, 0x10008}, /* 36/1.001 MHz */
        {PLL_CLK_36MHZ,  	 0x0b122100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_36_20MHZ,       0x0b122108, 0x10e10cf6, 0x10001, 0x10009},
        {PLL_CLK_36_23MHZ,       0x0b122108, 0x13f7ceda, 0x10001, 0x10009},
        {PLL_CLK_37_125D1001MHZ, 0x0b122108, 0x5cd5cd5d, 0x10001, 0x10009},
        {PLL_CLK_37_125MHZ,  	 0x0a152100, 0x0,        0x10001, 0x10004},
        {PLL_CLK_42D1001MHZ,  	 0x0d121108, 0xfc6b699f, 0x10001, 0x10006},
        {PLL_CLK_42MHZ,  	 0x0d121100, 0x0, 	 0x10001, 0x10006},
        {PLL_CLK_48D1001MHZ,  	 0x1f110108, 0xf7d0f16c, 0x10001, 0x10009},
        {PLL_CLK_48MHZ,  	 0x1f110100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_48_6MHZ,  	 0x1f110108, 0x66666667, 0x10001, 0x10009},
        {PLL_CLK_49_5D1001MHZ,   0x20110108, 0xf78f78f7, 0x10001, 0x10009}, /* 49.5/1.001 MHz */
        {PLL_CLK_49_5MHZ,  	 0x20110100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_54MHZ,  	 0x0f111100, 0x0,        0x10001, 0x10008},
        {PLL_CLK_54M1001MHZ,  	 0x0f111108, 0x04189375, 0x10001, 0x10008}, /* 54*1.001 MHz */
        {PLL_CLK_60MHZ,		 0x09113100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_60M1001MHZ,	 0x09113108, 0x028f5c29, 0x10001, 0x10009}, /* 60*1.001 MHz */
        {PLL_CLK_60_05MHz,	 0x09113108, 0x0253c6e0, 0x10001, 0x10009},
        {PLL_CLK_60_16MHZ,	 0x09113108, 0x07087088, 0x10001, 0x10009},
        {PLL_CLK_60_29MHZ,       0x09113108, 0x0c31a1f2, 0x10001, 0x10009},
        {PLL_CLK_60_33MHZ,	 0x09113108, 0x0e10e023, 0x10001, 0x10009},        
        {PLL_CLK_60_35MHZ,	 0x09113108, 0x0ec41dd2, 0x10001, 0x10009},
        {PLL_CLK_60_39MHZ,	 0x09113108, 0x10a3d70b, 0x10001, 0x10009},
	{PLL_CLK_64D1001MHZ,  	 0x0a113108, 0xa7f05079, 0x10001, 0x10009}, /* 64/1.001 MHz */
	{PLL_CLK_64MHZ,    	 0x0a113108, 0xaaaaaaaa, 0x10001, 0x10009},
        {PLL_CLK_74_25D1001MHZ,  0x0a133108, 0xfd2fd2fd, 0x10001, 0x10004}, /* 74.25/1.001 MHz */
        {PLL_CLK_74_25MHZ,  	 0x0a133100, 0x0,        0x10001, 0x10004},
        {PLL_CLK_90MHZ,  	 0x09122100, 0x0, 	 0x10001, 0x10003},
        {PLL_CLK_90_62D1001MHZ,  0x09122108, 0x0ef13846, 0x10001, 0x10003},
        {PLL_CLK_90_62MHZ, 	 0x09122108, 0x118468f8, 0x10001, 0x10003},
        {PLL_CLK_90_69D1001MHZ,  0x09122108, 0x10f97edd, 0x10001, 0x10003},
        {PLL_CLK_90_69MHZ,       0x09122108, 0x138d33e9, 0x10001, 0x10003},
        {PLL_CLK_96D1001MHZ,	 0x1f100108, 0xf7d0f16c, 0x10001, 0x10009},  /* 96/1.001 MHz */
        {PLL_CLK_96MHZ,  	 0x1f100100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_99_18D1001MHZ,  0x0a122108, 0x0251054d, 0x10001, 0x10003},
	{PLL_CLK_99_18MHZ,       0x0a122108, 0x0523f680, 0x10001, 0x10003},
	{PLL_CLK_108MHZ,  	 0x0b122100, 0x0,    	 0x10001, 0x10003},
        {PLL_CLK_148_5D1001MHZ,  0x20100108, 0xf78f78f7, 0x10001, 0x10006}, /* 148.5/1.001 MHz */
        {PLL_CLK_148_5MHZ, 	 0x20100100, 0x0,        0x10001, 0x10006},
        {PLL_CLK_216MHZ,  	 0x0b122108, 0x0,    	 0x10001, 0x10002},
        {0x0, 	    		 0x17100104, 0x0,        0x10001, 0x10018}  /* 27MHz as default */
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
	u32	frac_100khz; /* Frac PLL change for 100KHz */
};

/**
 * RCT register settings for VOUT pll with Fref = 27MHz
 */
static struct vout_rct_obj_s G_vout_rct[] = {

	/* Freq     		Ctrl 	    FRAC        PRE  	 POST  	  0
						S_RATIO 	FRAC_100KHZ */
        {PLL_CLK_27MHZ,  	0x0b131100, 0x0,        0x10001, 0x10006, 0x10001},
	{PLL_CLK_13_5D1001MHZ, 	0x0f131108, 0xfbe878b8, 0x10001, 0x10010, 0x10001},
	{PLL_CLK_13_5MHZ,  	0x0f131100, 0x0,    	0x10001, 0x10010, 0x10001},
	{PLL_CLK_18_44MHz,	0x0a011108, 0xed6a9264,	0x10001, 0x10010, 0x10001},
	{PLL_CLK_24_54MHZ,	0x0e131108, 0x8ba1f4b2, 0x10001, 0x10008, 0x10001},
	{PLL_CLK_27D1001MHZ,  	0x0f131108, 0xfbe878b6, 0x10001, 0x10008, 0x10001},
	{PLL_CLK_26_9485MHZ,	0x0b131108, 0xf831aa72, 0x10001, 0x10008, 0x10001}, /* - 1 LN at 60hz */
	{PLL_CLK_26_9568MHZ,	0x0b131108, 0xf9724745, 0x10001, 0x10008, 0x10001}, /* - 1 LN at 50hz */
	{PLL_CLK_27_0432MHZ,	0x0f131108, 0x068db8bb, 0x10001, 0x10008, 0x10001}, /* + 1 LN at 50hz */
	{PLL_CLK_27_0514MHZ,  	0x0f131108, 0x07cc3cb0, 0x10001, 0x10008, 0x10001}, /* + 1 LN at 60hz */
        {PLL_CLK_27M1001MHZ,  	0x0f131108, 0x04189375, 0x10001, 0x10008, 0x10001}, /* 27*1.001 MHz */
        {PLL_CLK_54MHZ,  	0x0b131100, 0x0,        0x10001, 0x10003, 0x10002},
        {PLL_CLK_74_25D1001MHZ, 0x0a133108, 0xfd2fd2fd, 0x10001, 0x10004, 0x10001}, /* 74.25/1.001 MHz */
        {PLL_CLK_74_25MHZ,  	0x0a133100, 0x0,        0x10001, 0x10004, 0x10001},
        {PLL_CLK_108MHZ,  	0x0b133100, 0x0,        0x10001, 0x10003, 0x10004},
        {PLL_CLK_148_5D1001MHZ,	0x20100108, 0xf78f78f7, 0x10001, 0x10006, 0x10001},
        {PLL_CLK_148_5MHZ, 	0x20100100, 0x0,    	0x10001, 0x10006, 0x10001}, /* Decrease VCO1 stress value */
        {0x0, 	    		0x17100104, 0x0,        0x10001, 0x10018, 0x10001}  /* 27MHz as default */
};

static int so_freq_index 	= 0;	/* Default freq = 27 MHz */
static int vout_freq_index[2] 	= {0};

/**
 * Configure sensor clock out
 */
void rct_set_so_freq_hz(u32 freq_hz)
{

	int i;
	u32 ctrl;
	int sw_int_frac = 0;

	for (i = 0; ;i++) {
	        if ((G_so_rct[i].freq == 0) || (G_so_rct[i].freq == freq_hz))
			break;
	}

	/* Check if the PLL mode is changed bet INT and FRAC modes */
	ctrl = readl(PLL_SENSOR_CTRL_REG) & 0x8; /* Check if INT mode */
	if (ctrl != (G_so_rct[i].sensor_ctrl & 0x8)) {
	    	sw_int_frac = 1;
	}

	so_freq_index = i;

	if (G_so_rct[i].sensor_frac != 0x00) {
		amba_outl(0x7017011c, 0x3ff10000);
		amba_outl(0x70170120, 0x00069300);
	} else {
		amba_outl(0x7017011c, 0x3f770000);
		amba_outl(0x70170120, 0x00068300);
	}

	if (sw_int_frac) {
		ctrl = readl(PLL_SENSOR_CTRL_REG);
		if (G_so_rct[i].sensor_frac != 0x00) {
			/* 1. change to Frac Mode(1) */
			ctrl |= 0x00000008;
		} else {
			/* 1. change to Int Mode(0) */
			ctrl &= ~0x00000008;
		}

		/* 2. change the pll_we(bit[0]) to disable(0) */
		ctrl &= ~0x01;
		writel(PLL_SENSOR_CTRL_REG, ctrl);

		/* 3. assert pll reset(bit[4]), 1ms */
		ctrl |= 0x10;
		writel(PLL_SENSOR_CTRL_REG, ctrl);
		dly_tsk(1);

		/* 4. deassert pll reset(bit[4]) */
	}

	/* Configure sensor clock out. */
	writel(PLL_SENSOR_CTRL_REG, G_so_rct[i].sensor_ctrl & (~0x10));
        writel(PLL_SENSOR_FRAC_REG, G_so_rct[i].sensor_frac);
	writel(SCALER_SENSOR_POST_REG, G_so_rct[i].scaler_post);

	dly_tsk(2);
	writel(SCALER_SENSOR_PRE_REG, 0x1003);
	writel(SCALER_SENSOR_PRE_REG, G_so_rct[i].scaler_pre);
	writel(PLL_SENSOR_CTRL_REG, G_so_rct[i].sensor_ctrl | 0x1);

        rct_alan_zhu_magic_loop(PLL_LOCK_SENSOR);
	so_clk_freq_hz = G_so_rct[i].freq;

}

/**
 *  Rescale the sensor clock frequency
 *  1 macrostep = 65536 microsteps
 */
void rct_rescale_so_pclk_freq_hz(u32 scale)
{
	rct_rescale_pll(PLL_SO, scale);
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
static void rct_set_vout_pll_onchip(int vo_id, u32 freq_hz)
{
	int i;
	u32 ctrl = 0;
	int sw_int_frac = 0;

	for (i = 0; ;i++) {
	        if ((G_vout_rct[i].freq == 0) ||
		    (G_vout_rct[i].freq == freq_hz))
			break;
	}

	/* Check if the PLL mode is changed bet INT and FRAC modes */
	if (vo_id == PLL_VIDEO) {
		ctrl = readl(PLL_VIDEO_CTRL_REG) & 0x8; /* Check if INT mode */
	} else {
                ctrl = readl(PLL_VIDEO2_CTRL_REG) & 0x8; /* Check if INT mode */
	}

	if (ctrl != (G_vout_rct[i].video_ctrl & 0x8)) {
	    	sw_int_frac = 1;
	}

	vout_freq_index[vo_id] = i;

	if (vo_id == PLL_VIDEO) {
		if (G_vout_rct[i].video_frac != 0x00) {
			amba_outl(0x70170130, 0x3ff10000);
			amba_outl(0x70170134, 0x00069300);
		} else {
			amba_outl(0x70170130, 0x3f770000);
			amba_outl(0x70170134, 0x00068300);
		}

		if (sw_int_frac) {
			ctrl = readl(PLL_VIDEO_CTRL_REG);
			if (G_vout_rct[i].video_frac != 0x00) {
				/* 1. change Frac Mode(1) */
				ctrl |= 0x00000008;
			} else {
				/* 1. change Int Mode(0) */
				ctrl &= ~0x00000008;
			}

			/* 2. change the pll_we(bit[0]) to disable(0) */
			ctrl &= ~0x01;
			writel(PLL_VIDEO_CTRL_REG, ctrl);

			/* 3. assert pll reset(bit[4]), 1ms */
			ctrl |= 0x10;
			writel(PLL_VIDEO_CTRL_REG, ctrl);
			dly_tsk(1);

			/* 4. deassert pll reset(bit[4]) */
		}

		/* Configure sensor clock out. The reference clock = 27 MHz */
		writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl & (~0x10));
		writel(PLL_VIDEO_FRAC_REG, G_vout_rct[i].video_frac);
		writel(SCALER_VIDEO_POST_REG, G_vout_rct[i].scaler_post);
		writel(SCALER_VIDEO_OS_RATION_REG, G_vout_rct[i].os_ratio);
		dly_tsk(1);
		writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl | 0x01);
		writel(SCALER_VIDEO_REG, G_vout_rct[i].scaler_pre);
		vo_clk_freq_hz = G_vout_rct[i].freq;

	} else {
		if (G_vout_rct[i].video_frac != 0x00) {
			amba_outl(0x7017013c, 0x3ff10000);
			amba_outl(0x70170140, 0x00069300);
		} else {
			amba_outl(0x7017013c, 0x3f770000);
			amba_outl(0x70170140, 0x00068300);
		}

		if (sw_int_frac) {
			ctrl = readl(PLL_VIDEO2_CTRL_REG);
			if (G_vout_rct[i].video_frac != 0x00) {
				/* 1. change to Frac Mode(1) */
				ctrl |= 0x00000008;
			} else {
				/* 1. change to Int Mode(0) */
				ctrl &= ~0x00000008;
			}

			/* 2. change the pll_we(bit[0]) to disable(0) */
			ctrl &= ~0x01;
			writel(PLL_VIDEO2_CTRL_REG, ctrl);

			/* 3. assert pll reset(bit[4]), 1ms */
			ctrl |= 0x10;
			writel(PLL_VIDEO2_CTRL_REG, ctrl);
			dly_tsk(1);

			/* 4. deassert pll reset(bit[4]) */
		}

		writel(PLL_VIDEO2_CTRL_REG, G_vout_rct[i].video_ctrl & (~0x10));
		writel(PLL_VIDEO2_FRAC_REG, G_vout_rct[i].video_frac);
		writel(SCALER_VIDEO2_POST_REG, G_vout_rct[i].scaler_post);
		dly_tsk(1);
		writel(PLL_VIDEO2_CTRL_REG, G_vout_rct[i].video_ctrl | 0x01);
		writel(SCALER_VIDEO2_REG, G_vout_rct[i].scaler_pre);
        	vo2_clk_freq_hz = G_vout_rct[i].freq;

	}
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
	u32	frac_100khz; /* Frac PLL change for 100KHz */
};

/**
 * RCT register settings for SO with Fref = 27MHz
 */
static struct vout_rct_pclk_obj_s G_vout_rct_pclk[] = {
	/* Fref			Freq
		Ctrl        	FRAC   	PRE  	POST  	0S_RATIO*/
        /* Fref = 13.5 MHz */
        {PLL_CLK_13_5MHZ, 	PLL_CLK_27D1001MHZ,
		0x0c013108, 	0xfcace213, 	0x10001, 0x1000d},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_27MHZ,
		0x0f012100, 	0x0, 		0x10001, 0x1000c},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_27M1001MHZ,
		0x0f012108, 	0x04189375,	0x10001, 0x1000c},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x0d013108, 	0xbc7bc7bc, 	0x10001, 0x10005},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_74_25MHZ,
		0x0d013108, 	0xc0000000, 	0x10001, 0x10005},

        /* Fref = 27 * 1.001 MHz */
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_27MHZ,
		0x0B011108, 	0xfcee5a88, 	0x10001, 0x1000c},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_27M1001MHZ,
		0x0B011100, 	0x0, 		0x10001, 0x1000c},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x0D011108, 	0xb8f875b2, 	0x10001, 0x10005},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_74_25MHZ,
		0x0D011108, 	0xbc7bc7bc, 	0x10001, 0x10005},
        {PLL_CLK_27M1001MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x0D001108, 	0xb8f875b2, 	0x10001, 0x10005},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_148_5MHZ,
		0x0D001108, 	0xbc7bc7bc, 	0x10001, 0x10005},

        /* Fref = 74.25/1.001 MHz */
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_27D1001MHZ,
        	0x08020108,  	0xba2e8ba2, 	0x10001, 0x10008},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_27MHZ,
		0x08020108,  	0xbc6a7ef9, 	0x10001, 0x10008},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_27M1001MHZ,
        	0x08020108,  	0xbea704bc, 	0x10001, 0x10008},
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_74_25D1001MHZ,
		0x08000100,  	0x0, 		0x10001, 0x10009},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_74_25MHZ,
		0x08000108,  	0x024dd2f2, 	0x10001, 0x10009},
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_148_5D1001MHZ,
		0x07000100,  	0x0, 		0x10001, 0x10004},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_148_5MHZ,
		0x07000108,  	0x020c49bb, 	0x10001, 0x10004},

        /* Fref = 74.25 MHz */
         {PLL_CLK_74_25MHZ, 	PLL_CLK_27D1001MHZ,
        	0x08020108,  	0xb7f32a91, 	0x10001, 0x10008},
        {PLL_CLK_74_25MHZ, 	PLL_CLK_27MHZ,
		0x08020108,  	0xba2e8ba2, 	0x10001, 0x10008},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_27M1001MHZ,
		0x08020108,  	0xbc6a7ef9, 	0x10001, 0x10008},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x08000108,  	0xfdb2c3e6, 	0x10001, 0x10009},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_74_25MHZ,
		0x08000100,  	0x0, 		0x10001, 0x10009},
        {PLL_CLK_74_25MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x07000108,  	0xfdf43c5b, 	0x10001, 0x10004},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_148_5MHZ,
		0x07000100,  	0x0, 		0x10001, 0x10004},

        /* Fref = 148.5/1.001 MHz */
        {PLL_CLK_148_5D1001MHZ, PLL_CLK_27D1001MHZ,
		0x04020108,  	0x5d1745d1, 	0x10001, 0x10008},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_27MHZ,
		0x04020108,  	0x5e353f7c, 	0x10001, 0x10008},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_27M1001MHZ,
        	0x04020108,  	0x5f53825e, 	0x10001, 0x10008},
        {PLL_CLK_148_5D1001MHZ, PLL_CLK_74_25D1001MHZ,
		0x04000108,  	0x80000000, 	0x10001, 0x10009},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_74_25MHZ,
		0x04000108,  	0x8126E978, 	0x10001, 0x10009},
        {PLL_CLK_148_5D1001MHZ, PLL_CLK_148_5D1001MHZ,
		0x04000100,  	0x0, 		0x10001, 0x10004},
	{PLL_CLK_148_5D1001MHZ, PLL_CLK_148_5MHZ,
		0x04000108,  	0x010624dd, 	0x10001, 0x10004},

        /* Fref = 148.5 MHz */
        {PLL_CLK_148_5MHZ, 	PLL_CLK_27D1001MHZ,
		0x03020108,  	0x5bf99549, 	0x10001, 0x10008},
        {PLL_CLK_148_5MHZ, 	PLL_CLK_27MHZ,
		0x03020108,  	0x5d1745d2, 	0x10001, 0x10008},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_27M1001MHZ,
		0x03020108,  	0x5e353f7d, 	0x10001, 0x10008},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x03000108,  	0x7ed961f4, 	0x10001, 0x10009},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_74_25MHZ,
		0x03000108,  	0x80000000, 	0x10001, 0x10009},
        {PLL_CLK_148_5MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x03000108,  	0xfefa1e2d, 	0x10001, 0x10004},
	{PLL_CLK_148_5MHZ, 	PLL_CLK_148_5MHZ,
		0x03000100,  	0x0, 		0x10001, 0x10004},

        /* 27MHz as default */
        {0x0, 	    		0,
		0x0,   		0x0,	0x0, 	0x0,  	0x0}
};

/**
 * The legal range of rescaling is ((-2^31) + 1) to ((2^31) - 1)
 */
int rct_rescale_pll(int id, u32 scale)
{
#define PLL_FRAC_MAX_VALUE	((u32) (0x1L) << 31)

	int sign = scale & 0x80000000;
	u32 frac_sign = 0, index;
        u32 rescale_frac = 0;
	u32 pllfrac = 0;
	int pllctrl = 0, pll_int = 0, pll_ctrl_changed = 0;
	u32 cur_clk_src = 0, pllfrac_reg = 0, pllctrl_reg = 0;
	int pllctrl2 = 0, pllctrl3 = 0;

        if  (cur_clk_src == VO_CLK_EXTERNAL) {
 		printk("Not supported for the external cloc");
		return -1;
	}

        if (id == PLL_VIDEO) {
                cur_clk_src 	= vo_clk_src;
		pllfrac_reg	= PLL_VIDEO_FRAC_REG;
		pllctrl_reg 	= PLL_VIDEO_CTRL_REG;
		pllctrl2 	= 0x70170130;
		pllctrl3 	= 0x70170134;
	} else if (id == PLL_VIDEO2) {
                cur_clk_src 	= vo_clk_src;
		pllfrac_reg	= PLL_VIDEO2_FRAC_REG;
		pllctrl_reg 	= PLL_VIDEO2_CTRL_REG;
		pllctrl2 	= 0x7017013c;
		pllctrl3 	= 0x70170140;
	} else if (id == PLL_SO) {
		cur_clk_src 	= 0;
		pllfrac_reg	= PLL_SENSOR_FRAC_REG;
		pllctrl_reg 	= PLL_SENSOR_CTRL_REG;
		pllctrl2 	= 0x7017011c;
		pllctrl3 	= 0x70170120;
	}

	writel(pllctrl2, 0x3ff10000);
	writel(pllctrl3, 0x00069300);

	if ((id == PLL_VIDEO) || (id == PLL_VIDEO2)) {
		index = vout_freq_index[id];
	        if ((cur_clk_src == VO_CLK_ONCHIP_PLL_CLK_SI) ||
	            (cur_clk_src == VO_CLK_ONCHIP_PLL_SP_CLK) ) {
	              	pllfrac 	= (G_vout_rct_pclk[index].video_frac &
			      		  			0x7fffffff);
	              	frac_sign	= G_vout_rct_pclk[index].video_frac >> 31;
	               	pllctrl 	= G_vout_rct_pclk[index].video_ctrl &
			       					0x00ffffff;
	               	pll_int 	= G_vout_rct_pclk[index].video_ctrl >> 24;
		} else {
		       	pllfrac 	= (G_vout_rct[index].video_frac &
			       		   			0x7fffffff);
		       	frac_sign 	= G_vout_rct[index].video_frac >> 31;
			pllctrl 	= G_vout_rct[index].video_ctrl &
					   			0x00ffffff;
	               	pll_int 	= G_vout_rct[index].video_ctrl >> 24;
		}
	} else if (id == PLL_SO) {
		pllfrac 	= (G_so_rct[so_freq_index].sensor_frac &
			   	   				0x7fffffff);
		frac_sign 	= G_so_rct[so_freq_index].sensor_frac >> 31;
		pllctrl 	= G_so_rct[so_freq_index].sensor_ctrl &
								0x00ffffff;
		pll_int 	= G_so_rct[so_freq_index].sensor_ctrl >> 24;
	}

	if ((readl(pllctrl_reg) & 0x8) == 0) { /* Check if it's on INT mode */
		/* 1. change to Frac Mode(1) */
		pllctrl |= 0x00000008;

		/* 2. change the pll_we(bit[0]) to disable(0) */
		pllctrl &= ~0x01;
		writel(pllctrl_reg, pllctrl);

		/* 3. assert pll reset(bit[4]), 1ms */
		pllctrl |= 0x10;
		writel(pllctrl_reg, pllctrl);

		/* 4. deassert pll reset(bit[4]) */
		pllctrl &= ~0x10;
	}

	rescale_frac 	= (scale & 0x7fffffff); /* # of microsteps */
	if (sign) {     /* Decrement */
  	        if (pllfrac >= rescale_frac) {
  		        rescale_frac 	= pllfrac - rescale_frac;
  		} else {
                        rescale_frac 	= 0x80000000 - rescale_frac + pllfrac;
                        frac_sign    	= 1;
		}
	} else {        /* Increment */
                rescale_frac += pllfrac; /* # of microsteps */
	}

	if (rescale_frac >= PLL_FRAC_MAX_VALUE) {
		/* 0.5 <= frac < 1 */
		if (frac_sign == 0) {
			pll_int++;
			frac_sign 		= 1;
			pll_ctrl_changed 	= 1;
		} else if (rescale_frac > PLL_FRAC_MAX_VALUE) {
		        pll_int--;
			frac_sign 		= 0;
			pll_ctrl_changed 	= 1;
		}

		if (pll_ctrl_changed)
			rescale_frac = (0xffffffff - rescale_frac + 1) &
								0x7fffffff;
	}

	/* PLL is on FRAC mode when the tuneup of clock rate is turned on */
	/* Frac PLL reg > 0x0 for Frac mode */
	if (rescale_frac == 0)
		rescale_frac = 0x1;

	rescale_frac += (frac_sign << 31);

	pllctrl |= (0x1 << 3);   	/* INTMOD = 1 */

	pllctrl += (pll_int << 24);
	writel(pllctrl_reg, pllctrl);

	writel(pllfrac_reg, rescale_frac);
	//printk("PLL rescale = 0x%x, 0x%x", rescale_frac, readl(pllfrac_reg));
        writel(pllctrl_reg, pllctrl | 0x1);

	return 0;
}

/* Configure video clock out with pixel clock or clock_si */
static void rct_set_vout_pll_spclk(int vo_id, u32 freq_hz)
{
	int i;

	if (spclk_freq_hz == PLL_CLK_27MHZ) {
	        rct_set_vout_pll_onchip(PLL_VIDEO, freq_hz);
	} else {
		for (i = 0; ;i++) {
		        if ((G_vout_rct_pclk[i].fref == 0) ||
			    ((G_vout_rct_pclk[i].fref == spclk_freq_hz) &&
			     (G_vout_rct_pclk[i].freq == freq_hz)) )
				break;
		}

		vout_freq_index[vo_id] = i;

		if (G_vout_rct_pclk[i].fref != 0) {
                        if (vo_id == PLL_VIDEO) {
                                writel(PLL_VIDEO_CTRL_REG,
			       		G_vout_rct_pclk[i].video_ctrl);
	        		writel(PLL_VIDEO_FRAC_REG,
			       		G_vout_rct_pclk[i].video_frac);
				writel(SCALER_VIDEO_POST_REG,
				       G_vout_rct_pclk[i].scaler_post);
	        		writel(SCALER_VIDEO_OS_RATION_REG,
				       G_vout_rct_pclk[i].os_ratio);

				if (G_vout_rct_pclk[i].video_frac != 0x0) {
					amba_outl(0x70170130, 0x3ff10000);
					amba_outl(0x70170134, 0x00069300);
				}
				writel(PLL_VIDEO_CTRL_REG,
			       		G_vout_rct_pclk[i].video_ctrl);
				writel(PLL_VIDEO_CTRL_REG,
					G_vout_rct_pclk[i].video_ctrl | 0x01);
				writel(SCALER_VIDEO_REG,
			       		G_vout_rct_pclk[i].scaler_pre);

				vo_clk_freq_hz = G_vout_rct_pclk[i].freq;
			} else {
                                writel(PLL_VIDEO2_CTRL_REG,
			       		G_vout_rct_pclk[i].video_ctrl);
	        		writel(PLL_VIDEO2_FRAC_REG,
			       		G_vout_rct_pclk[i].video_frac);
		        	writel(SCALER_VIDEO2_POST_REG,
				       G_vout_rct_pclk[i].scaler_post);

				if (G_vout_rct_pclk[i].video_frac != 0x0) {
					amba_outl(0x7017013c, 0x3ff10000);
					amba_outl(0x70170140, 0x00069300);
				}

				writel(PLL_VIDEO2_CTRL_REG,
			       		G_vout_rct_pclk[i].video_ctrl);
				writel(PLL_VIDEO2_CTRL_REG,
			       		G_vout_rct_pclk[i].video_ctrl | 0x01);
				writel(SCALER_VIDEO2_REG,
			       		G_vout_rct_pclk[i].scaler_pre);
				vo2_clk_freq_hz = G_vout_rct_pclk[i].freq;
			}
	        } else {
	                /* Fref = 27MHz as default */
	                rct_set_vout_pll_onchip(PLL_VIDEO, freq_hz);
		}
	}
}

static int rct_rescale_vout_pll(int vo_id, u32 scale)
{
	return (rct_rescale_pll(vo_id, scale));
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
 */
void rct_rescale_vout_clk_freq_hz(u32 scale)
{
	vo_clk_scaling = scale;
        rct_rescale_vout_pll(PLL_VIDEO, scale);
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
	vo2_clk_src = clk_src;
}

static void rct_config_vout_pll_reg (int vo_id, u32 freq_hz)
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

	if (vo_id == PLL_VIDEO2) {
		use_ext_clk 	= USE_EXTERNAL_VD2_CLK_REG;
		clk_ref_ext 	= CLK_REF_VIDEO2_EXTERNAL_REG;
		use_clk_si	= USE_CLK_SI_4_VO2_REG;
		use_clk_si_inp	= USE_CLK_SI_INPUT_MODE_REG;
		pll_ctrl	= PLL_VIDEO2_CTRL_REG;
		clk_src		= vo2_clk_src;
		vd_ctrl_reg	= VOUT2_CTL_CONTROL_REG;
	}

        switch (clk_src) {
	case VO_CLK_ONCHIP_PLL_CLK_SI:
		do {
                	rct_set_vout_pll_spclk(vo_id, freq_hz);

                	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x1);
	        	writeb(use_clk_si, 0x1);

	        	/* SI clock input mode */
	        	writeb(use_clk_si_inp, 1);

			if (vo_id == PLL_VIDEO)
        			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);
			else
				rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO2);

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
        case VO_CLK_ONCHIP_PLL_SP_CLK:
                do {
	        	rct_set_vout_pll_spclk(vo_id, freq_hz);
	        	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x1);
	        	writeb(use_clk_si, 0x0);
	        	writeb(use_clk_si_inp, 0); /* SI clock input mode */

			if (vo_id == PLL_VIDEO)
        			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);
			else
				rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO2);

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
			writeb(use_ext_clk, 0x1);

			if (vo_id == PLL_VIDEO)
        			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);
			else
				rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO2);

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
		if (vo_id == PLL_VIDEO)
			vo_clk_src = VO_CLK_ONCHIP_PLL_27MHZ;
		else
			vo2_clk_src = VO_CLK_ONCHIP_PLL_27MHZ;

		do {
			rct_set_vout_pll_onchip(vo_id, freq_hz);

                	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x0);
	        	writeb(use_clk_si, 0x0);
	        	writeb(use_clk_si_inp, 0); /* SI clock output mode */

			if (vo_id == PLL_VIDEO)
        			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);
			else
				rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO2);

                        if (readl(vd_ctrl_reg) != 0)
                                break;
			else {
			        retry--;
#ifdef VOUT_PLL_DEBUG
			        a5_retry++;
			        printk("Onchip : %d Retry(%d)", a5_retry, retry);
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
        rct_config_vout_pll_reg(PLL_VIDEO2, freq_hz);
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
 */
void rct_rescale_vout2_clk_freq_hz(u32 scale)
{
	vo2_clk_scaling = scale;
 	rct_rescale_vout_pll(PLL_VIDEO2, scale);
}

/**
 *  Rescale the VOUT clock frequency
 *  1 macrostep = 65536 microsteps
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
	rct_config_vout_pll_reg (PLL_VIDEO, freq_hz);
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
	writel(PLL_USB_CTRL_REG, 0x07081100);
	writel(PLL_USB_CTRL_REG, 0x07081101);
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

/**
 * Select HDMI clock source
 */
void rct_set_hdmi_clk_src(u32 clk_src)
{
        hdmi_clk_src = clk_src;

        switch (hdmi_clk_src) {

        /* FIXME */
        case	HDMI_CLK_ONCHIP_PLL:
                writeb(HDMI_CTRL_REG, (readb(HDMI_CTRL_REG) & (~0x5)));
                break;
	case HDMI_CLK_EXTERNAL:
	        writeb(HDMI_CTRL_REG, (readb(HDMI_CTRL_REG) & (~0x4)) | 0x1);
	        break;
	default: /* HDMI_CLK_PHY_CLK_VO, default setting */
	        writeb(HDMI_CTRL_REG, readb(HDMI_CTRL_REG) | 0x4);
	}
}

/**
 * Clock RCT runtime object. Clock output using ref Colck
 */
struct clk_rct_refclk_obj_s {
	u32 	fref;
	u32 	freq;
	u32 	ctrl;
	u32    	frac;
	u32    	scaler_pre;
	u32    	scaler_post;
};

/**
 * RCT register settings for VIN
 */
static struct clk_rct_refclk_obj_s G_vin_rct_spclk[] = {
	/* Fref			Freq
		Ctrl        	FRAC   	PRE  	POST */
        {PLL_CLK_240MHZ, 	PLL_CLK_320MHZ,
		0x07024100, 	0x0,	0x0001000a,	0x00010001},
	{PLL_CLK_240MHZ,  	PLL_CLK_384MHZ,
		0x0f033100, 	0x0, 	0x0001000a,	0x00010001},
        {PLL_CLK_240MHZ,  	PLL_CLK_192MHZ,
		0x0f073100, 	0x0, 	0x0001000a,	0x00010001},
	{PLL_CLK_240MHZ, 	PLL_CLK_160MHZ,
		0x09053100, 	0x0,	0x0001000a,	0x00010001},
	{PLL_CLK_120MHZ,  	PLL_CLK_80MHZ,
		0x09082100, 	0x0, 	0x00010005,	0x00010001},

        /* 27MHz as default */
        {0x0, 	    		0,
		0x0,   		0x0,	0x0, 	0x0}
};

u32 rct_cal_vin_freq_hz_slvs(u32 dclk, u32 act_lanes, u32 pel_width, u32 ddr)
{
	return ((dclk * act_lanes / pel_width) / (4 << ddr));
}

void rct_set_vin_freq_hz(u32 ref_freq_hz, u32 freq_hz)
{
	int i;

	for (i = 0; ;i++) {
	        if ((G_vin_rct_spclk[i].fref == 0) ||
		    ((G_vin_rct_spclk[i].fref == ref_freq_hz) &&
		     (G_vin_rct_spclk[i].freq == freq_hz)) )
			break;
	}

	if (G_vin_rct_spclk[i].fref == 0)
		i = 0;

	amba_outl(0x701701b8, 0x3f1f0000);
	amba_outl(0x701701bc, 0x00068300);

        writel(SCALER_VIN_PRE_REG,  G_vin_rct_spclk[i].scaler_pre);
        writel(SCALER_VIN_POST_REG, G_vin_rct_spclk[i].scaler_post);
	writel(PLL_VIN_CTRL_REG,    G_vin_rct_spclk[i].ctrl);
 	writel(PLL_VIN_FRAC_REG,    G_vin_rct_spclk[i].frac);
  	dly_tsk(1);
 	writel(PLL_VIN_CTRL_REG,    G_vin_rct_spclk[i].ctrl | 0x1);

	rct_alan_zhu_magic_loop(PLL_LOCK_VIN);
}

void rct_set_hdmi_phy_freq_hz(u32 freq_hz)
{
	writel(0x70170150, 0x3f770000);
	writel(0x70170154, 0x00068300);

	/* gclk_hdmi = 10 * gclk_vo when 0x7017008 bit-0 = 0*/
        writel(PLL_HDMI_CTRL_REG,    0x09011100);
        writel(PLL_HDMI_FRAC_REG,    0x0);
	writel(SCALER_HDMI_POST_REG, 0x0010001);
	writel(PLL_HDMI_CTRL_REG,    0x09011101);
	writel(SCALER_HDMI_PRE_REG,  0x0010001);

	rct_alan_zhu_magic_loop(PLL_LOCK_HDMI);
}

int get_ssi_clk_src(void)
{
	return (readb(CLK_REF_SSI_REG) & 0x1);
}

void rct_set_ssi_clk_src(int src)
{
	writeb(CLK_REF_SSI_REG, src & 0x1);
}

/*
 * Config the mode of LVDS I/O pads
 */
void rct_set_vin_lvds_pad(int mode)
{
	switch (mode) {
	case VIN_LVDS_PAD_MODE_LVDS:
		writel(LVDS_LVCMOS_REG, readl(LVDS_LVCMOS_REG) & ~(0xfffff));

		/* Set bit mode = 1 */
		writel(LVDS_ASYNC_REG, readl(LVDS_ASYNC_REG) | (0xf400000));
		break;
	case VIN_LVDS_PAD_MODE_SLVS:
		writel(LVDS_LVCMOS_REG, readl(LVDS_LVCMOS_REG) & ~(0xfffff));

		/* Clr bit mode */
		writel(LVDS_ASYNC_REG, readl(LVDS_ASYNC_REG) & ~(0xf000000));
		break;
	default: /* VIN_LVDS_PAD_MODE_LVCMOS */
		writel(LVDS_LVCMOS_REG, readl(LVDS_LVCMOS_REG) | 0xfffff);
		writel(LVDS_ASYNC_REG, 0x00000000);
	}
}

void rct_set_adc_clk_freq_hz(u32 freq_hz)
{
	register u32 clk_div;

        clk_div     = PLL_FREQ_HZ / freq_hz;
	writel(SCALER_ADC_REG, 0x0010000 | clk_div);
}
