/*
 * arch/arm/plat-ambarella/include/plat/pll.h
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

#ifndef __PLAT_AMBARELLA_PLL_H
#define __PLAT_AMBARELLA_PLL_H

/* ==========================================================================*/
#define AMBPLL_MAX_CMD_LENGTH			(32)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/
static inline unsigned long cpufreq_scale_copy(unsigned long old,
	u_int div, u_int mult)
{
#if BITS_PER_LONG == 32

	u64 result = ((u64) old) * ((u64) mult);
	do_div(result, div);
	return (unsigned long) result;

#elif BITS_PER_LONG == 64

	unsigned long result = old * ((u64) mult);
	result /= div;
	return result;

#endif
};

extern unsigned long loops_per_jiffy;
static inline unsigned int ambarella_adjust_jiffies(unsigned long val,
	unsigned int oldfreq, unsigned int newfreq)
{
	if (((val == AMBA_EVENT_PRE_CPUFREQ) && (oldfreq < newfreq)) ||
		((val == AMBA_EVENT_POST_CPUFREQ) && (oldfreq != newfreq))) {
		loops_per_jiffy = cpufreq_scale_copy(loops_per_jiffy,
			oldfreq, newfreq);

		return newfreq;
	}

	return oldfreq;
}

/* ==========================================================================*/
extern int ambarella_init_pll(void);
extern u32 ambarella_pll_suspend(u32 level);
extern u32 ambarella_pll_resume(u32 level);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

