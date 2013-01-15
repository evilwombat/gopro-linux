/*
 * arch/arm/plat-ambarella/include/mach/smp.h
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

#ifndef __ASM_ARCH_SMP_H
#define __ASM_ARCH_SMP_H

/* ==========================================================================*/
#include <asm/hardware/gic.h>
#include <asm/smp_twd.h>

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#define hard_smp_processor_id()			\
	({						\
		unsigned int cpunum;			\
		__asm__("mrc p15, 0, %0, c0, c0, 5"	\
			: "=r" (cpunum));		\
		cpunum &= 0x03;				\
	})

static inline void smp_cross_call(const struct cpumask *mask, int ipi)
{
	gic_raise_softirq(mask, ipi);
}

/* ==========================================================================*/
extern u32 arch_smp_suspend(u32 level);
extern u32 arch_smp_resume(u32 level);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __ASM_ARCH_SMP_H */

