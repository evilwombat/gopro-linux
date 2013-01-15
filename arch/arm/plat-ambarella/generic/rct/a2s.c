/**
 * system/src/peripheral/rct/a2s.c
 *
 * History:
 *    2008/05/13 - [Allen Wang] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifdef ENABLE_DEBUG_MSG_RCT
#define DEBUG_MSG		printk
#else
#define DEBUG_MSG(...)
#endif

//#define VOUT_PLL_DEBUG
#ifdef VOUT_PLL_DEBUG
static int sp_retry = 0;
static int a2_retry = 0;
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

	/*
	 * hacked video PLL
	 */
	/* video ctrl2 */
	amba_outl(0x70170130, 0x3f770000);

	/* Check if vout controller is running */
	if (readl(VOUT_CTL_CONTROL_REG) & 0xc0000000) {

		/* scalers */
		amba_outl(0x7017001c, 0x00010002);
		amba_outl(0x7017001c, 0x00010001);

		/* post-scalers, write a value different than default(0x10001),
		   then write back */
		amba_outl(0x701700a0, 0x10002);
		amba_outl(0x701700a0, 0x10001);

		/* pll ctrl reg (int, sdiv, sout) */
		amba_outl(0x70170014, 0x0a130104);
		amba_outl(0x70170014, 0x0a130105);
		do_some_delay();

		/* Check video clock is locked or not */
		val = readl(VOUT_CTL_CLUT_REG);

	}

	/*
	 * Miscellaneous...
	 */
	amba_outl(0x70170030, 0x10003);
  	amba_outl(0x70170030, 0x10006);
  	amba_outl(0x70170024, 0x0b031100);
  	do_some_delay();
  	amba_outl(0x70170024, 0x0b031101);

	/*
	 * init ms PLL
	 */
	writel(SCALER_MS_REG,
	       (readl(SCALER_MS_REG) & 0xffff0000) | 0x1);
	writel(CG_DVEN_REG, 0);
}

/**
 * Check the PLL lock status (A3) or wait for the PLL clock to lock (A2)
 */
void rct_alan_zhu_magic_loop(int clk_chk)
{
	u8 reg = readb(PLL_LOCK_REG) & clk_chk;
	register u32 loop_cnt = (get_core_bus_freq_hz() / 500);

	while ((reg == 0) && loop_cnt) {
                reg = readb(PLL_LOCK_REG) & clk_chk;
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

static u32 get_core_pll_freq(u32 z, u32 f, u32 c)
{
	u32 intprog, sout, sdiv;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		return 0;
	}

	intprog += 1 + z;
	sdiv++;
	sout++;

	return PLL_FREQ_HZ * intprog * sdiv / sout / 2;
}

u32 get_core_bus_freq_hz(void)
{
#if     defined(__FPGA__)
	return 48000000;
#else
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);
	u32 c = readl(PLL_CORE_CTRL_REG);

	return get_core_pll_freq(z, f, c);
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
	if ((readl(ADC16_CTRL_REG) & 0x1) == 0) {
		return PLL_FREQ_HZ / readl(SCALER_ADC_REG);
	} else {
		return get_audio_freq_hz();
	}
#endif
}

void rct_set_adc_clk_src(int src)
{
	writel(ADC16_CTRL_REG, (src & 0x01));
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
	return (get_apb_bus_freq_hz() / readl(CG_SSI_REG));
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
	return get_apb_bus_freq_hz();
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

u32 get_ms_freq_hz(void)
{
	u32 scaler = readl(SCALER_MS_REG) & MS_INTEGER_DIV;
	return (get_core_bus_freq_hz() * 2 / scaler);
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
	*chip  = 0x2;
	*major = 0x1;
	*minor = 0x0;
}

/* FIRMWARE_CONTAINER_TYPE */
#define SDMMC_TYPE_AUTO		0x0
#define SDMMC_TYPE_SD		0x1
#define SDMMC_TYPE_SDHC		0x2
#define SDMMC_TYPE_MMC		0x3
#define SDMMC_TYPE_MOVINAND	0x4

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
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_AUTO)
		rval |= BOOT_FROM_SDMMC;
#endif
#endif
		return rval;
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
	}

	if ((val & SYS_CONFIG_SPI_BOOT) != 0x0) {
		rval |= BOOT_FROM_SPI;
		rval |= get_sm_boot_device();
	}

	if (((val & SYS_CONFIG_FLASH_BOOT) == 0x0) &&
		((val & SYS_CONFIG_SPI_BOOT) == 0x0)) {
		/* USB boot */

		/* Force enabling flash access on USB boot */
#if defined(FIRMWARE_CONTAINER)
		rval |= BOOT_FROM_SPI;
		rval |= get_sm_boot_device();
#else
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
#endif	/* FIRMWARE_CONTAINER */
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
	/* Not support in A2S. */
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

	DEBUG_MSG("SD Freq = %dhz, Set SCALER_SD48_REG 0x%x", freq_hz, scaler);
}

void rct_set_ir_pll(void)
{
	writew(CG_IR_REG, 0x800);
}

void rct_set_ssi_pll(void)
{
	writel(CG_SSI_REG, get_apb_bus_freq_hz() / 13500000);
}

void rct_set_ms_pll(u32 freq_hz)
{
#define DUTY_CYCLE_CONTRL_ENABLE	0x01000000 /* Duty cycle correction */

	u32 scaler;
	u32 core_clk;

	core_clk = get_core_bus_freq_hz();
	scaler = ((core_clk << 1) / freq_hz) + 1;

	/* msclk = core_freq * 2 / Int_div */
	/* For example: msclk = 108 * 2 / 16 = 13.5 Mhz */
	writel(SCALER_MS_REG,
	       (readl(SCALER_MS_REG) & 0xffff0000) |
	       (DUTY_CYCLE_CONTRL_ENABLE | scaler));

	DEBUG_MSG("MS Freq = %dhz, Set SCALER_MS_REG 0x%x", freq_hz, scaler);
}

void rct_enable_ms(void)
{
	writel(MS_EN_REG, 0x1);
}

void rct_disable_ms(void)
{
	writel(MS_EN_REG, 0x0);
}

void rct_set_ms_delay(void)
{
	u32 ms_dly_ctr = readl(MS_DELAY_CTRL_REG);

#ifdef MS_READ_TIME_ADJUST
	ms_dly_ctr &= 0x0fffffff;
	ms_dly_ctr |= (MS_READ_TIME_ADJUST << 28);
#endif

#ifdef MS_DATA_OUTPUT_DELAY
	ms_dly_ctr &= 0xf0ffffff;
	ms_dly_ctr |= (MS_DATA_OUTPUT_DELAY << 24);
#endif

#ifdef MS_DATA_INPUT_DELAY
	ms_dly_ctr &= 0xfff0ffff;
	ms_dly_ctr |= (MS_DATA_INPUT_DELAY << 16);
#endif

#ifdef MS_SCLK_OUTPUT_DELAY
	ms_dly_ctr &= 0xfffff0ff;
	ms_dly_ctr |= (MS_SCLK_OUTPUT_DELAY << 8);
#endif

	writel(MS_DELAY_CTRL_REG, ms_dly_ctr);
}

void rct_set_dven_clk(u32 freq_hz)
{
	u32 ref_clk = 27000000;
	u32 val;

	val = ref_clk / freq_hz;

	writel(CG_DVEN_REG, val);
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
        {PLL_CLK_27MHZ,  	 0x09111100, 0x0,        0x10001, 0x1000a},
	{PLL_CLK_10MHZ, 	 0x13111100, 0x00000000, 0x10001, 0x10036},
	{PLL_CLK_13D1001MHZ,	 0x16020108, 0x1688AE5D, 0x10001, 0x10010},
	{PLL_CLK_13MHZ,		 0x16020108, 0x1C71C71D, 0x10001, 0x10010},
        {PLL_CLK_13_5D1001MHZ, 	 0x0f131108, 0xfbe878b8, 0x10001, 0x10010},
	{PLL_CLK_13_5MHZ,  	 0x0f131100, 0x0,    	 0x10001, 0x10010},
	{PLL_CLK_22_5MHZ,  	 0x09153100, 0x0, 	 0x10001, 0x10008},
 	{PLL_CLK_24D1001MHZ,  	 0x0d131108, 0x35406b4d, 0x10001, 0x10008},
	{PLL_CLK_24MHZ,  	 0x1f110100, 0x0, 	 0x10001, 0x10012},
	{PLL_CLK_24_3MHZ,  	 0x0d131108, 0x66666667, 0x10001, 0x10008},
        {PLL_CLK_25MHZ,  	 0x0e131108, 0xd097b425, 0x10001, 0x10008},
	{PLL_CLK_27D1001MHZ,  	 0x0f110108, 0xfbe878b6, 0x10001, 0x10008},
	{PLL_CLK_26_9485MHZ,	 0x09111108, 0xfb1f0a87, 0x10001, 0x1000a}, /* - 1 LN at 60hz */
	{PLL_CLK_26_9568MHZ,	 0x09111108, 0xfbe76c8b, 0x10001, 0x1000a}, /* - 1 LN at 50hz */
	{PLL_CLK_27_0432MHZ,	 0x09111108, 0x04189375, 0x10001, 0x1000a}, /* + 1 LN at 50hz */
	{PLL_CLK_27_0514MHZ,  	 0x09111108, 0x04dfa5ee, 0x10001, 0x1000a}, /* + 1 LN at 60hz */
	{PLL_CLK_27M1001MHZ,  	 0x09111108, 0x04189375, 0x10001, 0x1000a}, /* 27*1.001 MHz */
        {PLL_CLK_36D1001MHZ, 	 0x14131108, 0x4fe0a0f3, 0x10001, 0x10008}, /* 36/1.001 MHz */
        {PLL_CLK_36MHZ,  	 0x0b122100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_37_125D1001MHZ, 0x0b122108, 0x5cd5cd5d, 0x10001, 0x10009},
        {PLL_CLK_37_125MHZ,  	 0x0a152100, 0x0,        0x10001, 0x10004},
	{PLL_CLK_42D1001MHZ,  	 0x0d121108, 0xfc6b699f, 0x10001, 0x10006},
	{PLL_CLK_42MHZ,  	 0x0d121100, 0x0, 	 0x10001, 0x10006},
	{PLL_CLK_45MHZ,  	 0x09122100, 0x0, 	 0x10001, 0x10006},
	{PLL_CLK_48D1001MHZ,  	 0x1f110108, 0xf7d0f16c, 0x10001, 0x10009},
        {PLL_CLK_48MHZ,  	 0x1f110100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_48_6MHZ,  	 0x1f110108, 0x66666667, 0x10001, 0x10009},
        {PLL_CLK_49_5D1001MHZ,   0x20110108, 0xf78f78f7, 0x10001, 0x10009}, /* 49.5/1.001 MHz */
        {PLL_CLK_49_5MHZ,  	 0x20110100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_54MHZ,  	 0x0f111100, 0x0,        0x10001, 0x10008},
        {PLL_CLK_54M1001MHZ,  	 0x0f111108, 0x04189375, 0x10001, 0x10008}, /* 54*1.001 MHz */
        {PLL_CLK_60MHZ,		 0x09113100, 0x0, 	 0x10001, 0x10009},
        {PLL_CLK_60M1001MHZ,	 0x09113108, 0x028f5c29, 0x10001, 0x10009}, /* 60*1.001 MHz */
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
};

/**
 * RCT register settings for VOUT pll with Fref = 27MHz
 */
static struct vout_rct_obj_s G_vout_rct[] = {

	/* Freq     		Ctrl 	    FRAC        PRE  POST  0S_RATIO*/
	/* VCO = 10* Fvout for HDMI */
        {PLL_CLK_27MHZ,  	0x09111100, 0x0,        0x10001, 0x1000a, 0x10001},
	{PLL_CLK_13_5D1001MHZ, 	0x0e110108, 0xfbe878b7, 0x10001, 0x10010, 0x10001},
	{PLL_CLK_13_5MHZ,  	0x0b111100, 0x0,    	0x10001, 0x10018, 0x10001},
	{PLL_CLK_24_54MHZ,	0x0e131108, 0x8ba1f4b2, 0x10001, 0x10008, 0x10001},
	{PLL_CLK_27D1001MHZ,  	0x09111108, 0xfbe878b7, 0x10001, 0x1000a, 0x10001},
	{PLL_CLK_26_9485MHZ,	0x09111108, 0xfb1f0a87, 0x10001, 0x1000a, 0x10001}, /* - 1 LN at 60hz */
	{PLL_CLK_26_9568MHZ,	0x09111108, 0xfbe76c8b, 0x10001, 0x1000a, 0x10001}, /* - 1 LN at 50hz */
	{PLL_CLK_27_0432MHZ,	0x09111108, 0x04189375, 0x10001, 0x1000a, 0x10001}, /* + 1 LN at 50hz */
	{PLL_CLK_27_0514MHZ,  	0x09111108, 0x04dfa5ee, 0x10001, 0x1000a, 0x10001}, /* + 1 LN at 60hz */
        {PLL_CLK_27M1001MHZ,  	0x09111108, 0x04189375, 0x10001, 0x1000a, 0x10001}, /* 27*1.001 MHz */
        {PLL_CLK_54MHZ,  	0x17100100, 0x0,        0x10001, 0x1000c, 0x10002},
        {PLL_CLK_74_25D1001MHZ, 0x0a114108, 0xfd2fd2fd, 0x10001, 0x1000a, 0x10001}, /* 74.25/1.001 MHz */
        {PLL_CLK_74_25MHZ,  	0x0a114100, 0x0,        0x10001, 0x1000a, 0x10001},
        {PLL_CLK_108MHZ,  	0x17100100, 0x0,    	0x10001, 0x10006, 0x10001},
        {PLL_CLK_148_5D1001MHZ,	0x14110108, 0xfa5fa5fb, 0x10001, 0x10002, 0x10001},
        {PLL_CLK_148_5MHZ, 	0x15110100, 0x0,    	0x10001, 0x10002, 0x10001}, /* Decrease VCO1 stress value */
        {0x0, 	    		0x17100100, 0x0,        0x10001, 0x10018, 0x10001}  /* 27MHz as default */
};

static int so_freq_index 	= 0;
static int vout_freq_index[2] 	= {0};

/**
 * Configure sensor clock out
 */
void rct_set_so_freq_hz(u32 freq_hz)
{
	int i;
	u32 ctrl = 0;
	int sw_int_frac = 0;

	for (i = 0; ;i++) {
	        if ((G_so_rct[i].freq == 0) || (G_so_rct[i].freq == freq_hz))
			break;
	}

	/* Check if the PLL mode is changed bet INT and FRAC modes */
	if (((G_so_rct[so_freq_index].sensor_frac != 0) 	&&
	     (G_so_rct[i].sensor_frac == 0)) 			||
	     ((G_so_rct[so_freq_index].sensor_frac == 0) 	&&
	     (G_so_rct[i].sensor_frac != 0))) {
	    	sw_int_frac = 1;
	}

	so_freq_index = i;

	if (G_so_rct[i].sensor_frac != 0x00) {
		amba_outl(0x7017011c, 0x00f10000);
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
	if (((G_vout_rct[vout_freq_index[vo_id]].video_frac != 0) 	&&
	     (G_vout_rct[i].video_frac == 0)) 				||
	     ((G_vout_rct[vout_freq_index[vo_id]].video_frac == 0) 	&&
	     (G_vout_rct[i].video_frac != 0))) {
	    	sw_int_frac = 1;
	}

	vout_freq_index[vo_id] = i;

	ctrl = readl(PLL_VIDEO_CTRL_REG);
	if (G_vout_rct[i].video_frac != 0x00) {
		amba_outl(0x70170130, 0x00f10000);
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
	dly_tsk(1);
	writel(PLL_VIDEO_CTRL_REG, G_vout_rct[i].video_ctrl | 0x01);
	writel(SCALER_VIDEO_REG, G_vout_rct[i].scaler_pre);
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
		0x0f019c02, 	0xfbe8,	0x1001,	0x1004,	0x1001},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_27MHZ,
		0x10019c0a, 	0x0, 	0x1001, 0x1004, 0x1001},
	{PLL_CLK_13_5MHZ,  	PLL_CLK_27M1001MHZ,
		0x1000e902, 	0x0419,	0x1001, 0x1001,  0x1},

        /* Fref = 27 * 1.001 MHz */
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_27MHZ,
		0x0f00e902, 	0xfbe8, 0x1001,	0x02,  	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_27M1001MHZ,
		0x1000e90a, 	0x0,	0x1001,	0x02,  	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x05017a02, 	0x7d30, 0x1001, 	0x02, 	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_74_25MHZ,
		0x05017a02, 	0x7e97, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_27M1001MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x05017a02, 	0x7d30, 0x1001, 	0x01, 	0x1},
	{PLL_CLK_27M1001MHZ,  	PLL_CLK_148_5MHZ,
		0x05017a02, 	0x7e97, 0x1001, 	0x01,  	0x1},

        /* Fref = 48 MHz */
	{PLL_CLK_48MHZ,  	PLL_CLK_27MHZ,
		0x0902dc0a, 	0x0,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_48MHZ,  	PLL_CLK_27M1001MHZ,
		0x0900fa02, 	0x024d,	0x1001, 	0x01,  	0x2},
	{PLL_CLK_48MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x0c03d902, 	0x5cd6,	0x1001, 	0x08,  	0x1},
	{PLL_CLK_48MHZ,  	PLL_CLK_74_25MHZ,
		0x0c03d902, 	0x6000,	0x1001, 	0x08,  	0x1},
        {PLL_CLK_48MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x0c03d902, 	0x5cd6,	0x1001, 	0x04,  	0x1},
	{PLL_CLK_48MHZ,  	PLL_CLK_148_5MHZ,
		0x0c03d902, 	0x6000,	0x1001, 	0x04,  	0x1},

        /* Fref = 49.5 MHz */
        {PLL_CLK_49_5MHZ,  	PLL_CLK_27MHZ,
		0x06015a0a, 	0x0,	0x1001, 	0x0b,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_27M1001MHZ,
		0x06015a02, 	0x0189,	0x1001, 	0x0b,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_74_25D1001MHZ,
		0x05005a02, 	0xfe77,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_74_25MHZ,
		0x06005a0a, 	0x0,	0x1001, 	0x02,  	0x1},
        {PLL_CLK_49_5MHZ,  	PLL_CLK_148_5D1001MHZ,
		0x05005a02, 	0xfe77,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_49_5MHZ,  	PLL_CLK_148_5MHZ,
		0x06005a0a, 	0x0,	0x1001, 	0x01,  	0x1},

        /* Fref = 74.25/1.001 MHz */
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_27D1001MHZ,
		0x0803dc0a, 	0x0,	0x1001, 	0x0b,  	0x2},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_27MHZ,
		0x0803d902, 	0x020c, 0x1001, 	0x0b,  	0x2},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_27M1001MHZ,
        	0x0802cc02, 	0x0419,	0x1001, 	0x0b,  	0x1},
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_74_25D1001MHZ,
		0x0803cc0a, 	0x0,	0x1001, 	0x08,  	0x1},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_74_25MHZ,
		0x0803cc02, 	0x020c,	0x1001, 	0x08,  	0x1},
        {PLL_CLK_74_25D1001MHZ, PLL_CLK_148_5D1001MHZ,
		0x0803cc0a, 	0x0,	0x1001, 	0x04,  	0x1},
	{PLL_CLK_74_25D1001MHZ, PLL_CLK_148_5MHZ,
		0x0803cc02, 	0x020c,	0x1001, 	0x04,  	0x1},

        /* Fref = 74.25 MHz */
        {PLL_CLK_74_25MHZ, 	PLL_CLK_27MHZ,
		0x0803dc0a, 	0x0,	0x1001, 	0x0b,  	0x2},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_27M1001MHZ,
		0x0802fa02, 	0x020c,	0x1001, 	0x0b,  	0x1},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1001, 	0x08,  	0x1},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_74_25MHZ,
		0x0803cc0a, 	0x0,	0x1001, 	0x08,  	0x1},
        {PLL_CLK_74_25MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0703cc02, 	0xfdf4,	0x1001, 	0x04,  	0x1},
	{PLL_CLK_74_25MHZ, 	PLL_CLK_148_5MHZ,
		0x0803cc0a, 	0x0,	0x1001, 	0x04,  	0x1},

        /* Fref = 90615840/1.001 MHz */
        {PLL_CLK_90_62D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc5ab,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc6e4,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8e1e,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8fcc, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_90_62D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8e1e,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_90_62D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8fcc, 0x1001, 	0x01,  	0x1},

        /* Fref = 90615840 MHz */
        {PLL_CLK_90_62MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc473,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc5ab,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8c71,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8e1e, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_90_62MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8c71,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_90_62MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8e1e, 0x1001, 	0x01,  	0x1},

        /* Fref = 90687360/1.001 MHz */
        {PLL_CLK_90_69D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc4b5,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc5ed,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8ccb,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8e78, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_90_69D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8ccb,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_90_69D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8e78, 0x1001, 	0x01,  	0x1},

        /* Fref = 90687360 MHz */
        {PLL_CLK_90_69MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0xc37d,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0xc4b5,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x8b1e,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x8ccb, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_90_69MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x8b1e,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_90_69MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x8ccb, 0x1001, 	0x01,  	0x1},

        /* Fref = 95992800/1.001 MHz */
        {PLL_CLK_95_993D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x813d,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0x8264,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601cc02, 	0x301e,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x31b4, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_95_993D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x301e,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_95_993D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x31b4, 0x1001, 	0x01,  	0x1},

	/* Fref = 96/1.001 MHz */
        {PLL_CLK_96D1001MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x8127,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0x824e,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601fa02, 	0x3000,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x3196, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_96D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601cc02, 	0x3000,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_96D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x3196, 0x1001, 	0x01,  	0x1},

	/* Fref = 96 MHz */
        {PLL_CLK_96MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x8000,	0x1001, 	0x04,  	0x2},
	{PLL_CLK_96MHZ, 	PLL_CLK_27M1001MHZ,
		0x0401fa02, 	0x8126,	0x1001, 	0x02,  	0x2},
	{PLL_CLK_96MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601fa02, 	0x2e6b,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_96MHZ, 	PLL_CLK_74_25MHZ,
		0x0601fa02, 	0x3000, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_96MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601fa02, 	0x2e6b,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_96MHZ, 	PLL_CLK_148_5MHZ,
		0x0601fa02, 	0x3000, 0x1001, 	0x01,  	0x1},

        /* Fref = 99/1.001 MHz */
        {PLL_CLK_99D1001MHZ, 	PLL_CLK_27MHZ,
		0x0602da02, 	0x0189,	0x1001, 	0x0b,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_27M1001MHZ,
		0x0602da02, 	0x0312,	0x1001, 	0x0b,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0601da0a, 	0x0,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_74_25MHZ,
		0x0601da02, 	0x0189, 0x1001, 	0x02,  	0x1},
        {PLL_CLK_99D1001MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0601da0a, 	0x0,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_99D1001MHZ, 	PLL_CLK_148_5MHZ,
		0x0601da02, 	0x0189, 0x1001, 	0x01,  	0x1},

        /* Fref = 99 MHz */
        {PLL_CLK_99MHZ, 	PLL_CLK_27MHZ,
		0x0402d902, 	0x5c2d, 0x1001, 	0x04,  	0x2},
	{PLL_CLK_99MHZ, 	PLL_CLK_27M1001MHZ,
		0x0602da02, 	0x0189,	0x1001, 	0x0b,  	0x1},
	{PLL_CLK_99MHZ, 	PLL_CLK_74_25D1001MHZ,
		0x0501de02, 	0xfe77,	0x1001, 	0x02,  	0x1},
	{PLL_CLK_99MHZ, 	PLL_CLK_74_25MHZ,
		0x0601da0a, 	0x0,	0x1001, 	0x02,  	0x1},
        {PLL_CLK_99MHZ, 	PLL_CLK_148_5D1001MHZ,
		0x0501de02, 	0xfe77,	0x1001, 	0x01,  	0x1},
	{PLL_CLK_99MHZ, 	PLL_CLK_148_5MHZ,
		0x0601da0a, 	0x0,	0x1001, 	0x01,  	0x1},

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
	int pllctrl = 0, pll_int = 0;
	u32 cur_clk_src = 0, pllfrac_reg = 0, pllctrl_reg = 0;

        if (id == PLL_VIDEO) {
                cur_clk_src 	= vo_clk_src;
		pllfrac_reg	= PLL_VIDEO_FRAC_REG;
		pllctrl_reg 	= PLL_VIDEO_CTRL_REG;
	} else if (id == PLL_SO) {
		cur_clk_src 	= 0;
		pllfrac_reg	= PLL_SENSOR_FRAC_REG;
		pllctrl_reg 	= PLL_SENSOR_CTRL_REG;
	}

        if  (cur_clk_src == VO_CLK_EXTERNAL) {
 		printk("Not supported for the external cloc");
		return -1;
	}


	if (id == PLL_VIDEO) {
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
				pllctrl = G_vout_rct[index].video_ctrl &
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

	rescale_frac 	= (scale & 0x7fffffff); /* # of microsteps */
	if (sign) {     /* Decrement */
  	        if (pllfrac >= rescale_frac) {
  		        rescale_frac = pllfrac - rescale_frac;
  		} else {
                        rescale_frac = pllfrac +
				       (0xffffffff - rescale_frac + 1);
                        pll_int--;
			K_ASSERT(pllctrl >= 0);
		}
	} else {        /* Increment */
                rescale_frac += pllfrac; /* # of microsteps */
	}

	if (rescale_frac >= PLL_FRAC_MAX_VALUE) {
		/* 0.5 <= frac < 1 */
		if (frac_sign == 0) {
			pll_int++;
			frac_sign 	= 1;
		} else {
			frac_sign 	= 0;
		}
		rescale_frac   -= PLL_FRAC_MAX_VALUE;
	}

	rescale_frac += (frac_sign << 31);
	writel(pllfrac_reg, rescale_frac);
	if (rescale_frac)
        	pllctrl    |= (0x1 << 3);   	/* INTMOD = 1 */
	else
	        pllctrl    &= ~(0x1 << 3);   	/* INTMOD = 0 */

	pllctrl += (pll_int << 24);
        writel(pllctrl_reg, pllctrl);
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
					amba_outl(0x70170130, 0x00f10000);
					amba_outl(0x70170134, 0x00069300);
				}
				writel(PLL_VIDEO_CTRL_REG,
			       		G_vout_rct_pclk[i].video_ctrl);
				writel(PLL_VIDEO_CTRL_REG,
					G_vout_rct_pclk[i].video_ctrl | 0x01);
				writel(SCALER_VIDEO_REG,
			       		G_vout_rct_pclk[i].scaler_pre);

				vo_clk_freq_hz = G_vout_rct_pclk[i].freq;
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

#if (VOUT_DISPLAY_SECTIONS == 1)
	u32 vd_ctrl_reg		= VOUT_CTL_CONTROL_REG;
#endif
#if (VOUT_DISPLAY_SECTIONS == 2)
	u32 vd_ctrl_reg		= VOUT_DB_CONTROL_REG;
#endif

        switch (clk_src) {
	case VO_CLK_ONCHIP_PLL_CLK_SI:
		do {
                	rct_set_vout_pll_spclk(vo_id, freq_hz);

                	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x1);
	        	writeb(use_clk_si, 0x1);

	        	/* SI clock input mode */
	        	writeb(use_clk_si_inp, 1);

			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);

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
	        	rct_set_vout_pll_spclk(vo_id, freq_hz);
	        	writeb(use_ext_clk, 0x0);
	        	writeb(clk_ref_ext, 0x1);
	        	writeb(use_clk_si, 0x0);
	        	writeb(use_clk_si_inp, 0); /* SI clock input mode */

			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);

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

			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);

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

			rct_alan_zhu_magic_loop(PLL_LOCK_VIDEO);

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
	return vo2_clk_scaling;
}

/**
 * Configure video clock out
 */
void rct_set_vout_freq_hz(u32 freq_hz)
{
	amba_outl(0x70170130, 0x00770000);
	amba_outl(0x70170134, 0x00068300);
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

void rct_set_usb_phy_pll(void)
{
	register u32 clk;

	/* Program the USB phy clock generator */
	clk = get_core_bus_freq_hz();

	if (clk == 216000000) {
		/* Core == 216MHz, */
		writel(USB_REFCLK_REG,readl(USB_REFCLK_REG) | 0x01 );
		writel(SCALER_USB_REG,
			(readl(SCALER_USB_REG) & 0xffe0ff00) |
			 0x00100012
			);
	}
	else if (clk == 162000000){
		/* Core == 162MHz, */
		writel(USB_REFCLK_REG,readl(USB_REFCLK_REG) | 0x01 );
		writel(SCALER_USB_REG,
			(readl(SCALER_USB_REG) & 0xffe0ff00) |
			 0x0001001b
			);
	}
	else
	{
		printk("need external clock for usb phy");
		/* set Core == 216MHz, */
		writel(USB_REFCLK_REG,readl(USB_REFCLK_REG) | 0x01 );
		writel(SCALER_USB_REG,
			(readl(SCALER_USB_REG) & 0xffe0ff00) |
			 0x00100012
			);
	}

}

/* called by prusb driver */
void _init_usb_pll(void)
{
	rct_set_usb_phy_pll();
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
 * RCT register settings for SO with Fref = 27MHz
 */
static struct clk_rct_refclk_obj_s G_vin_rct_spclk[] = {
	/* Fref			Freq
		Ctrl        	FRAC   	PRE  	POST */
        /* Fref = 13.5 MHz */
        {PLL_CLK_240MHZ, 	PLL_CLK_320MHZ,
		0x0f017100, 	0x0,	0x00010030,	0x00010001},
	{PLL_CLK_240MHZ,  	PLL_CLK_384MHZ,
		0x0f01f100, 	0x0, 	0x00010050,	0x00010001},

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
		i = 1; /* 27MHz */

	amba_outl(0x701701b8, 0x3f770000);
	amba_outl(0x701701bc, 0x00068300);

	writel(SCALER_VIN_PRE_REG,  0x0010003);
	writel(SCALER_VIN_PRE_REG,  G_vin_rct_spclk[i].scaler_pre);
	writel(PLL_VIN_FRAC_REG,    G_vin_rct_spclk[i].frac);
	writel(PLL_VIN_CTRL_REG,    G_vin_rct_spclk[i].ctrl);
	writel(PLL_VIN_CTRL_REG,    G_vin_rct_spclk[i].ctrl | 0x1);
	dly_tsk(1);
	writel(SCALER_VIN_POST_REG, 0x0010002);
	writel(SCALER_VIN_POST_REG, G_vin_rct_spclk[i].scaler_post);
}

void rct_set_hdmi_phy_freq_hz(u32 freq_hz)
{
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
}

void rct_set_adc_clk_freq_hz(u32 freq_hz)
{
	register u32 clk_div;

        clk_div     = PLL_FREQ_HZ / freq_hz;
	writel(SCALER_ADC_REG, 0x0010000 | clk_div);
}
