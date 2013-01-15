/*
 * arch/arm/plat-ambarella/generic/clk.c
 *
 * History:
 *	2011/07/05 - [Zhenwu Xue] Initial Version
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/delay.h>
#include <plat/clk.h>
#include <plat/rct.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#define DO_DIV_ROUND(divident, divider)     do {	\
	(divident) += ((divider)>>1);			\
	do_div( (divident) , (divider) );		\
	} while (0)

static LIST_HEAD(all_clocks);
DEFINE_SPINLOCK(lock);

static int default_independant_clk_enable(struct clk *c)
{
	unsigned int	val;

	val = amba_readl(c->ctrl_reg);
	val &= 0xffffff9f;
	val |= 0x00000001;
	amba_writel(c->ctrl_reg, val);
	val &= 0xfffffffe;
	amba_writel(c->ctrl_reg, val);

	return 0;
}

static int default_independant_clk_disable(struct clk *c)
{
	unsigned int	val;

	val = amba_readl(c->ctrl_reg);
	val |= 0x00000061;
	amba_writel(c->ctrl_reg, val);
	val &= 0xfffffffe;
	amba_writel(c->ctrl_reg, val);

	return 0;
}

static unsigned long default_independant_clk_get_rate(struct clk *c)
{
	unsigned int		ctrl_val;
	unsigned int		refclk, preScaler, postScaler;
	unsigned int		pll_int, pll_frac, frac, sdiv, sout;
	unsigned long long	rate, rate1, rate2, divident, divider;

	ctrl_val	= amba_readl(c->ctrl_reg);
	refclk		= 24000000;
	if (c->pres_reg != PLL_REG_UNAVAILABLE) {
		preScaler	= amba_readl(c->pres_reg);
	} else {
		preScaler	= 0;
	}
	if (c->post_reg != PLL_REG_UNAVAILABLE) {
		postScaler	= amba_readl(c->post_reg);
	} else {
		postScaler	= 0;
	}
	pll_int		= (ctrl_val & 0x7f000000) >> 24;
	pll_frac	= amba_readl(c->frac_reg);
	sdiv		= (ctrl_val & 0x0000f000) >> 12;
	sout		= (ctrl_val & 0x000f0000) >> 16;

	rate		= refclk;
	divident	= rate * (pll_int + 1) * (sdiv + 1);
	divider		= preScaler * (sout + 1) * postScaler;
	DO_DIV_ROUND(divident, divider);
	rate1		= divident;
	if (pll_frac & 0x80000000) {
		/* Negative */
		frac	= 0x80000000 - (pll_frac & 0x7fffffff);
		divident= rate * frac * (sdiv + 1);
		divider	= preScaler * (sout + 1) * postScaler;
		DO_DIV_ROUND(divident, divider);
		rate2	= divident;
		rate	= rate1 - (rate2 >> 32);
	} else {
		/* Positive */
		frac	= pll_frac & 0x7fffffff;
		divident= rate * frac * (sdiv + 1);
		divider	= preScaler * (sout + 1) * postScaler;
		DO_DIV_ROUND(divident, divider);
		rate2	= divident;
		rate	= rate1 + (rate2 >> 32);
	}

	c->rate		= rate;
	return rate;
}

static int default_independant_clk_set_rate(struct clk *c, unsigned long rate)
{
	unsigned int	ctrl, ctrl2 = 0, ctrl3 = 0, frac, preScaler, postScaler;
	unsigned int	old_fm, new_fm, reset_pll = 0;
	unsigned int	pll_lock, loop;

	if (c->rate == rate) {
		return 0;
	}

	ctrl	= amba_readl(c->ctrl_reg);
	old_fm	= (ctrl & 0x8) >> 3;

	switch (rate) {
	case PLL_CLK_10D1001MHZ:
		ctrl		= 0x7e122108;
		frac		= 0x8a466587;
		preScaler	= 0x00000004;
		postScaler	= 0x0000004c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_10MHZ:
		ctrl		= 0x7e122108;
		frac		= 0xaaab5271;
		preScaler	= 0x00000004;
		postScaler	= 0x0000004c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_13D1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x8a870f0c;
		preScaler	= 0x00000004;
		postScaler	= 0x0000003a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_13MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xaaab25b4;
		preScaler	= 0x00000004;
		postScaler	= 0x0000003a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_13_5D1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xdfc66a6c;
		preScaler	= 0x00000004;
		postScaler	= 0x00000038;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_13_5MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000038;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_17_97515MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xd379d90a;
		preScaler	= 0x00000004;
		postScaler	= 0x0000002a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_17_97554MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xd42cc2d7;
		preScaler	= 0x00000004;
		postScaler	= 0x0000002a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_17_98201MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xdfc6b8b7;
		preScaler	= 0x00000004;
		postScaler	= 0x0000002a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_18_44MHz:
		ctrl		= 0x7d122108;
		frac		= 0x01b514d8;
		preScaler	= 0x00000004;
		postScaler	= 0x00000029;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_22_5MHZ:
		ctrl		= 0x7e122108;
		frac		= 0x8000431b;
		preScaler	= 0x00000004;
		postScaler	= 0x00000022;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_23_9772MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xe1dfa5a6;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_23_9784MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xe375a1b5;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_23_99MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xfdb8966e;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_23_967MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xd4ded821;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_23_971MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xda3adcbd;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_24D1001MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xe049a998;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_23_996MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xfb59226a;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_24MHZ:
		ctrl		= 0x7b133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_24M1001MHZ:
		ctrl		= 0x7b133108;
		frac		= 0x1fbe76c8;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_24_072MHZ:
		ctrl		= 0x7b133108;
		frac		= 0x5f3bbb08;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_24_3MHZ:
		ctrl		= 0x7c122108;
		frac		= 0x8cccee5a;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_24_54MHZ:
		ctrl		= 0x7e122108;
		frac		= 0xd16c1b6b;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_25MHZ:
		ctrl		= 0x7c122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001e;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_27D1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xdfc6b8b7;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_26_9823MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xeae03b3f;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_26_9485MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xc286f8ae;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_26_9568MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xcc63f142;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_27MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_27_0432MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x339c0ebe;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_27_0514MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x3d688377;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_27M1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x20418937;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_30MHZ:
		ctrl		= 0x7c122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000019;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_27_1792MHZ:
		ctrl		= 0x7e122108;
		frac		= 0xd618d95d;
		preScaler	= 0x00000004;
		postScaler	= 0x0000001c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_32_5D1001MHZ:
		ctrl		= 0x7b133108;
		frac		= 0x7578b370;
		preScaler	= 0x00000004;
		postScaler	= 0x00000017;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_36D1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xdfc6b8b7;
		preScaler	= 0x00000004;
		postScaler	= 0x00000015;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_36MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000015;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_36_20MHZ:
		ctrl		= 0x7e122108;
		frac		= 0xb13b18db;
		preScaler	= 0x00000004;
		postScaler	= 0x00000015;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_36_23MHZ:
		ctrl		= 0x7e122108;
		frac		= 0xd1aa0caf;
		preScaler	= 0x00000004;
		postScaler	= 0x00000015;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_37_125D1001MHZ:
		ctrl		= 0x74133108;
		frac		= 0x71ef30a4;
		preScaler	= 0x00000004;
		postScaler	= 0x00000013;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_37_125MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xc0000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000014;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_42D1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0xdfc6b8b7;
		preScaler	= 0x00000004;
		postScaler	= 0x00000012;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_42MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000012;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_45MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000010;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_48D1001MHZ:
		ctrl		= 0x77133108;
		frac		= 0xe14f8b59;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_48MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_48_6MHZ:
		ctrl		= 0x79133108;
		frac		= 0x80000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_49_5D1001MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xa059f2bb;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_49_5MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xc0000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000f;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_54MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000e;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_54M1001MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x20418937;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000e;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60M1001MHZ:
		ctrl		= 0x77133108;
		frac		= 0x1eb851eb;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_05MHz:
		ctrl		= 0x77133108;
		frac		= 0x1bed527e;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_16MHZ:
		ctrl		= 0x77133108;
		frac		= 0x546540cc;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_18MHZ:
		ctrl		= 0x77133108;
		frac		= 0x5dfc5cdd;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_29MHZ:
		ctrl		= 0x77133108;
		frac		= 0x92539756;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_33MHZ:
		ctrl		= 0x77133108;
		frac		= 0xa8ca8198;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_35MHZ:
		ctrl		= 0x78133108;
		frac		= 0xb13165d4;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_39MHZ:
		ctrl		= 0x78133108;
		frac		= 0xc7ae147b;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_60_48MHz:
		ctrl		= 0x78133108;
		frac		= 0xf5c28f5d;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000c;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_64D1001MHZ:
		ctrl		= 0x6a133108;
		frac		= 0x8f632688;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_64MHZ:
		ctrl		= 0x6a133108;
		frac		= 0xaaaaaaab;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_65D1001MHZ:
		ctrl		= 0x76133108;
		frac		= 0x0c30c23f;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000b;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_72D1001MHZ:
		ctrl		= 0x77133108;
		frac		= 0xe14f8b59;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_72MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_74_25D1001MHZ:
		ctrl		= 0x6e133108;
		frac		= 0x438433d6;
		preScaler	= 0x00000004;
		postScaler	= 0x00000009;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_74_25MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xc0000000;
		preScaler	= 0x00000004;
		postScaler	= 0x0000000a;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_80MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000009;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_90D1001MHZ:
		ctrl		= 0x77133108;
		frac		= 0xe14f8b59;
		preScaler	= 0x00000004;
		postScaler	= 0x00000008;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_90MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000008;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_90_62D1001MHZ:
		ctrl		= 0x78133108;
		frac		= 0xb34ea343;
		preScaler	= 0x00000004;
		postScaler	= 0x00000008;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_90_62MHZ:
		ctrl		= 0x78133108;
		frac		= 0xd234eb9b;
		preScaler	= 0x00000004;
		postScaler	= 0x00000008;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_90_69D1001MHZ:
		ctrl		= 0x78133108;
		frac		= 0xcbb1f256;
		preScaler	= 0x00000004;
		postScaler	= 0x00000008;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_90_69MHZ:
		ctrl		= 0x78133108;
		frac		= 0xea9e6eec;
		preScaler	= 0x00000004;
		postScaler	= 0x00000008;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_95_993D1001MHZ:
		ctrl		= 0x6f133108;
		frac		= 0xe1355742;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_96D1001MHZ:
		ctrl		= 0x6f133108;
		frac		= 0xe35b4edc;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_96MHZ:
		ctrl		= 0x6f133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_99D1001MHZ:
		ctrl		= 0x73133108;
		frac		= 0x627631b6;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_99MHZ:
		ctrl		= 0x73133108;
		frac		= 0x80000864;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_99_18D1001MHZ:
		ctrl		= 0x73133108;
		frac		= 0x9852c009;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_99_18MHZ:
		ctrl		= 0x73133108;
		frac		= 0xb5f9a49d;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_108MHZ:
		ctrl		= 0x7d122108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000007;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_148_5D1001MHZ:
		ctrl		= 0x62144108;
		frac		= 0xe6ae66f8;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_148_5MHZ:
		ctrl		= 0x7b133108;
		frac		= 0xc0000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000005;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_120MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000006;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_160MHZ:
		ctrl		= 0x6a133108;
		frac		= 0xaaaaaaab;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_192MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_216MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_230_4MHZ:
		ctrl		= 0x77133108;
		frac		= 0x99999999;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_240MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_288MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_320MHZ:
		ctrl		= 0x77133108;
		frac		= 0x55555555;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	case PLL_CLK_384MHZ:
		ctrl		= 0x77133108;
		frac		= 0x00000000;
		preScaler	= 0x00000004;
		postScaler	= 0x00000004;
		ctrl2		= 0x3ff10000;
		ctrl3		= 0x00069300;
		break;

	default:
		printk("%s %d: Unsupported PLL %s Clock: %lu Hz!\n",
					__func__, __LINE__, c->name, rate);
		return -1;
	}

	new_fm	= (ctrl & 0x8) >> 3;
	if (old_fm != new_fm) {
		reset_pll	= 1;
	}
	if (rate > c->rate && rate - c->rate > PLL_RESET_THRESHOLD) {
		reset_pll	= 1;
	}
	if (rate < c->rate && c->rate - rate > PLL_RESET_THRESHOLD) {
		reset_pll	= 1;
	}
	if (reset_pll) {
		/* Reset PLL */
		amba_writel(c->ctrl_reg, ctrl | 0x10);
		amba_writel(c->ctrl_reg, ctrl | 0x11);
	}
	if (old_fm != new_fm) {
		if (c->ctrl2_reg != PLL_REG_UNAVAILABLE) {
			amba_writel(c->ctrl2_reg, ctrl2);
		}
		if (c->ctrl3_reg != PLL_REG_UNAVAILABLE) {
			amba_writel(c->ctrl3_reg, ctrl3);
		}
	}
	if (c->pres_reg != PLL_REG_UNAVAILABLE) {
		if (amba_readl(c->pres_reg) != preScaler) {
			amba_writel(c->pres_reg, preScaler);
		}
	}
	if (c->post_reg != PLL_REG_UNAVAILABLE) {
		if (amba_readl(c->post_reg) != postScaler) {
			amba_writel(c->post_reg, postScaler);
		}
	}
	if (amba_readl(c->frac_reg) != frac) {
		amba_writel(c->frac_reg, frac);
	}
	amba_writel(c->ctrl_reg, ctrl);
	amba_writel(c->ctrl_reg, ctrl | 0x01);
	amba_writel(c->ctrl_reg, ctrl);
	mdelay(1);

	if (c->lock_reg != PLL_REG_UNAVAILABLE) {
		loop = 0;
		do {
			pll_lock = amba_readl(c->lock_reg);
			pll_lock = (pll_lock >> c->lock_bit) & 0x1;
			loop++;
		} while(!pll_lock && loop < 100000);
		if (!pll_lock) {
			printk("%s %d: PLL %s is not locked!\n",
					__func__, __LINE__, c->name);
		}
	}
	c->rate		= rate;

	return 0;
}

static unsigned long default_divider_clk_get_rate(struct clk *c)
{
	if (c->parent && c->parent->ops &&
		c->parent->ops->get_rate && c->divider) {
		c->rate = c->parent->ops->get_rate(c->parent) / c->divider;
		return c->rate;
	} else {
		return 0;
	}
}

static struct clk_ops default_independant_clk_ops = {
	.enable		= default_independant_clk_enable,
	.disable	= default_independant_clk_disable,
	.get_rate	= default_independant_clk_get_rate,
	.round_rate	= NULL,
	.set_rate	= default_independant_clk_set_rate,
	.set_parent	= NULL,
};

static struct clk_ops default_divider_clk_ops = {
	.enable		= NULL,
	.disable	= NULL,
	.get_rate	= default_divider_clk_get_rate,
	.round_rate	= NULL,
	.set_rate	= NULL,
	.set_parent	= NULL,
};

/* Core */
static struct clk core_clk = {
	.parent		= NULL,
	.name		= "core",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_CORE_CTRL_REG,
	.pres_reg	= SCALER_CORE_PRE_REG,
#if (CHIP_REV != A2)
	.post_reg	= SCALER_CORE_POST_REG,
#else
	.post_reg	= PLL_REG_UNAVAILABLE,
#endif
	.frac_reg	= PLL_CORE_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_CORE_CTRL2_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
#endif
	.ctrl3_reg	= PLL_CORE_CTRL3_REG,
#if (CHIP_REV != A2)
	.lock_reg	= PLL_LOCK_REG,
#else
	.lock_reg	= PLL_REG_UNAVAILABLE,
#endif
	.lock_bit	= 6,
	.ops		= &default_independant_clk_ops,
};

/* AHB */
static struct clk ahb_clk = {
	.parent		= &core_clk,
	.name		= "ahb",
	.usage		= 0,
	.rate		= 0,
	.divider	= 2,
	.ops		= &default_divider_clk_ops,
};

/* APB */
static struct clk apb_clk = {
	.parent		= &core_clk,
	.name		= "apb",
	.usage		= 0,
	.rate		= 0,
	.divider	= 4,
	.ops		= &default_divider_clk_ops,
};

/* DRAM */
static struct clk dram_clk = {
	.parent		= NULL,
	.name		= "dram",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_DDR_CTRL_REG,
	.pres_reg	= PLL_REG_UNAVAILABLE,
	.post_reg	= PLL_REG_UNAVAILABLE,
	.frac_reg	= PLL_DDR_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_DDR_CTRL2_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
#endif
	.ctrl3_reg	= PLL_DDR_CTRL3_REG,
#if (CHIP_REV != A2)
	.lock_reg	= PLL_LOCK_REG,
#else
	.lock_reg	= PLL_REG_UNAVAILABLE,
#endif
	.lock_bit	= 5,
	.ops		= &default_independant_clk_ops,
};

/* IDSP */
static struct clk idsp_clk = {
	.parent		= NULL,
	.name		= "idsp",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_IDSP_CTRL_REG,
	.pres_reg	= PLL_REG_UNAVAILABLE,
#if (CHIP_REV == I1)
	.post_reg	= SCALER_IDSP_POST_REG,
#else
	.post_reg	= PLL_REG_UNAVAILABLE,
#endif
	.frac_reg	= PLL_IDSP_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_IDSP_CTRL2_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
#endif
	.ctrl3_reg	= PLL_IDSP_CTRL3_REG,
#if (CHIP_REV != A2)
	.lock_reg	= PLL_LOCK_REG,
#else
	.lock_reg	= PLL_REG_UNAVAILABLE,
#endif
	.lock_bit	= 4,
	.ops		= &default_independant_clk_ops,
};

/* ARM */
static struct clk arm_clk = {
	.parent		= NULL,
	.name		= "arm",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* Sensor */
static struct clk sensor_clk = {
	.parent		= NULL,
	.name		= "sensor",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_SENSOR_CTRL_REG,
	.pres_reg	= SCALER_SENSOR_PRE_REG,
	.post_reg	= SCALER_SENSOR_POST_REG,
	.frac_reg	= PLL_SENSOR_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_SENSOR_CTRL2_REG,
	.ctrl3_reg	= PLL_SENSOR_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
	.ctrl3_reg	= PLL_REG_UNAVAILABLE,
	.lock_reg	= PLL_REG_UNAVAILABLE,
#endif
	.lock_bit	= 3,
	.ops		= &default_independant_clk_ops,
};

/* Vout0 */
#if (CHIP_REV == A2)

static struct clk vout0_clk = {
	.parent		= NULL,
	.name		= "vout0",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_VIDEO_CTRL_REG,
	.pres_reg	= SCALER_VIDEO_REG,
	.post_reg	= SCALER_VIDEO_POST_REG,
	.frac_reg	= PLL_VIDEO_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_VIDEO_CTRL2_REG,
	.ctrl3_reg	= PLL_VIDEO_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
	.ctrl3_reg	= PLL_REG_UNAVAILABLE,
	.lock_reg	= PLL_REG_UNAVAILABLE,
#endif
	.lock_bit	= 1,
	.ops		= &default_independant_clk_ops,
};

#else

static struct clk vout0_clk = {
	.parent		= NULL,
	.name		= "vout0",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_VIDEO2_CTRL_REG,
	.pres_reg	= SCALER_VIDEO2_REG,
	.post_reg	= SCALER_VIDEO2_POST_REG,
	.frac_reg	= PLL_VIDEO2_FRAC_REG,
	.ctrl2_reg	= PLL_VIDEO2_CTRL2_REG,
	.ctrl3_reg	= PLL_VIDEO2_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 0,
	.ops		= &default_independant_clk_ops,
};

#endif

#if (CHIP_REV != A2)
/* Vout1 */
static struct clk vout1_clk = {
	.parent		= NULL,
	.name		= "vout1",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_VIDEO_CTRL_REG,
	.pres_reg	= SCALER_VIDEO_REG,
	.post_reg	= SCALER_VIDEO_POST_REG,
	.frac_reg	= PLL_VIDEO_FRAC_REG,
	.ctrl2_reg	= PLL_VIDEO_CTRL2_REG,
	.ctrl3_reg	= PLL_VIDEO_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 1,
	.ops		= &default_independant_clk_ops,
};

/* HDMI PHY */
static struct clk hdmi_phy_clk = {
	.parent		= NULL,
	.name		= "hdmi_phy",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_HDMI_CTRL_REG,
	.pres_reg	= SCALER_HDMI_PRE_REG,
	.post_reg	= SCALER_HDMI_POST_REG,
	.frac_reg	= PLL_HDMI_FRAC_REG,
	.ctrl2_reg	= PLL_HDMI_CTRL2_REG,
	.ctrl3_reg	= PLL_HDMI_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 8,
	.ops		= &default_independant_clk_ops,
};
#endif

/* SSI */
static struct clk ssi_clk = {
	.parent		= &apb_clk,
	.name		= "ssi",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* SSI2 */
static struct clk ssi2_clk = {
	.parent		= &apb_clk,
	.name		= "ssi2",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* UART */
static struct clk uart_clk = {
	.parent		= NULL,
	.name		= "uart",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* MOTOR */
static struct clk motor_clk = {
	.parent		= NULL,
	.name		= "motor",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* IR */
static struct clk ir_clk = {
	.parent		= NULL,
	.name		= "ir",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* SD */
static struct clk sd_clk = {
	.parent		= &core_clk,
	.name		= "sd",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* Audio */
static struct clk audio_clk = {
	.parent		= NULL,
	.name		= "audio",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_AUDIO_CTRL_REG,
	.pres_reg	= SCALER_AUDIO_PRE_REG,
	.post_reg	= PLL_REG_UNAVAILABLE,
	.frac_reg	= PLL_AUDIO_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_AUDIO_CTRL2_REG,
	.ctrl3_reg	= PLL_AUDIO_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
	.ctrl3_reg	= PLL_REG_UNAVAILABLE,
	.lock_reg	= PLL_REG_UNAVAILABLE,
#endif
	.lock_bit	= 7,
	.ops		= &default_independant_clk_ops,
};

/* PWM */
static struct clk pwm_clk = {
	.parent		= NULL,
	.name		= "pwm",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* USB PHY */
static struct clk usb_phy_clk = {
	.parent		= NULL,
	.name		= "usb_phy",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_USB_CTRL_REG,
	.pres_reg	= PLL_REG_UNAVAILABLE,
	.post_reg	= PLL_REG_UNAVAILABLE,
	.frac_reg	= PLL_USB_FRAC_REG,
#if (CHIP_REV != A2)
	.ctrl2_reg	= PLL_USB_CTRL2_REG,
	.ctrl3_reg	= PLL_USB_CTRL3_REG,
#else
	.ctrl2_reg	= PLL_REG_UNAVAILABLE,
	.ctrl3_reg	= PLL_REG_UNAVAILABLE,
#endif
	.ops		= &default_independant_clk_ops,
};

/* ADC */
static struct clk adc_clk = {
	.parent		= NULL,
	.name		= "adc",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

#if (CHIP_REV == I1)

/* Cortex */
static struct clk cortex_clk = {
	.parent		= NULL,
	.name		= "cortex",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_CORTEX_CTRL_REG,
	.pres_reg	= PLL_REG_UNAVAILABLE,
	.post_reg	= PLL_REG_UNAVAILABLE,
	.frac_reg	= PLL_CORTEX_FRAC_REG,
	.ctrl2_reg	= PLL_CORTEX_CTRL2_REG,
	.ctrl3_reg	= PLL_CORTEX_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 2,
	.ops		= &default_independant_clk_ops,
};

/* AXI */
static struct clk axi_clk = {
	.parent		= &cortex_clk,
	.name		= "axi",
	.usage		= 0,
	.rate		= 0,
	.divider	= 3,
	.ops		= &default_divider_clk_ops,
};

/* DDD */
static struct clk ddd_clk = {
	.parent		= NULL,
	.name		= "ddd",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_DDD_CTRL_REG,
	.pres_reg	= PLL_REG_UNAVAILABLE,
	.post_reg	= SCALER_DDD_POST_REG,
	.frac_reg	= PLL_DDD_FRAC_REG,
	.ctrl2_reg	= PLL_DDD_CTRL2_REG,
	.ctrl3_reg	= PLL_DDD_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 9,
	.ops		= &default_independant_clk_ops,
};

/* SSI_AHB */
static struct clk ssi_ahb_clk = {
	.parent		= &apb_clk,
	.name		= "ssi_ahb",
	.usage		= 0,
	.rate		= 0,
	.ops		= NULL,
};

/* SDXC */
static struct clk sdxc_clk = {
	.parent		= NULL,
	.name		= "sdxc",
	.usage		= 0,
	.rate		= 0,
	.ctrl_reg	= PLL_SDXC_CTRL_REG,
	.pres_reg	= PLL_REG_UNAVAILABLE,
	.post_reg	= SCALER_SDXC_POST_REG,
	.frac_reg	= PLL_SDXC_FRAC_REG,
	.ctrl2_reg	= PLL_SDXC_CTRL2_REG,
	.ctrl3_reg	= PLL_SDXC_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 10,
	.ops		= &default_independant_clk_ops,
};

#endif

struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk		*p;
	struct clk		*clk = ERR_PTR(-ENOENT);

	spin_lock(&lock);
	list_for_each_entry(p, &all_clocks, list) {
		if (strcmp(id, p->name) == 0) {
			clk = p;
			break;
		}
	}
	spin_unlock(&lock);

	return clk;
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);

int clk_enable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL) {
		return -EINVAL;
	}

	clk_enable(clk->parent);

	spin_lock(&lock);
	if ((clk->usage++) == 0 && clk->ops && clk->ops->enable) {
		(clk->ops->enable)(clk);
	}
	spin_unlock(&lock);

	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || clk == NULL) {
		return;
	}

	spin_lock(&lock);
	if ((--clk->usage) == 0 && clk->ops && clk->ops->disable) {
		(clk->ops->disable)(clk);
	}
	spin_unlock(&lock);

	clk_disable(clk->parent);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (IS_ERR(clk)) {
		return 0;
	}

	if (clk->rate != 0) {
		return clk->rate;
	}

	if (clk->ops != NULL && clk->ops->get_rate != NULL) {
		return (clk->ops->get_rate)(clk);
	}

	if (clk->parent != NULL) {
		return clk_get_rate(clk->parent);
	}

	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (!IS_ERR(clk) && clk->ops && clk->ops->round_rate) {
		return (clk->ops->round_rate)(clk, rate);
	}

	return rate;
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int		ret;

	if (IS_ERR(clk)) {
		return -EINVAL;
	}

	if (clk->ops == NULL || clk->ops->set_rate == NULL) {
		return -EINVAL;
	}

	spin_lock(&lock);
	ret = (clk->ops->set_rate)(clk, rate);
	spin_unlock(&lock);

	return ret;
}
EXPORT_SYMBOL(clk_set_rate);

struct clk *clk_get_parent(struct clk *clk)
{
	return clk->parent;
}
EXPORT_SYMBOL(clk_get_parent);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int		ret = 0;

	if (IS_ERR(clk)) {
		return -EINVAL;
	}

	spin_lock(&lock);
	if (clk->ops && clk->ops->set_parent) {
		ret = (clk->ops->set_parent)(clk, parent);
	}
	spin_unlock(&lock);

	return ret;
}
EXPORT_SYMBOL(clk_set_parent);

int ambarella_register_clk(struct clk *clk)
{
	spin_lock(&lock);
	list_add(&clk->list, &all_clocks);
	spin_unlock(&lock);

	return 0;
}

int __init ambarella_init_clk(void)
{
	ambarella_register_clk(&core_clk);
	ambarella_register_clk(&ahb_clk);
	ambarella_register_clk(&apb_clk);
	ambarella_register_clk(&dram_clk);
	ambarella_register_clk(&idsp_clk);
	ambarella_register_clk(&arm_clk);
	ambarella_register_clk(&sensor_clk);
	ambarella_register_clk(&vout0_clk);
#if (CHIP_REV != A2)
	ambarella_register_clk(&vout1_clk);
	ambarella_register_clk(&hdmi_phy_clk);
#endif
	ambarella_register_clk(&ssi_clk);
	ambarella_register_clk(&ssi2_clk);
	ambarella_register_clk(&uart_clk);
	ambarella_register_clk(&motor_clk);
	ambarella_register_clk(&ir_clk);
	ambarella_register_clk(&sd_clk);
	ambarella_register_clk(&audio_clk);
	ambarella_register_clk(&pwm_clk);
	ambarella_register_clk(&usb_phy_clk);
	ambarella_register_clk(&adc_clk);
#if (CHIP_REV == I1)
	ambarella_register_clk(&cortex_clk);
	ambarella_register_clk(&axi_clk);
	ambarella_register_clk(&ddd_clk);
	ambarella_register_clk(&ssi_ahb_clk);
	ambarella_register_clk(&sdxc_clk);
#endif

	return 0;
}
