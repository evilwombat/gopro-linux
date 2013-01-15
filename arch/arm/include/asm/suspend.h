/*
 * arch/arm/include/asm/suspend.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
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

#ifndef _ASM_ARM_SUSPEND_H
#define _ASM_ARM_SUSPEND_H

static inline int arch_prepare_suspend(void) { return 0; }
extern int arch_pfn_is_nosave(unsigned long pfn);
extern void arch_copy_data_page(unsigned long dst_pfn, unsigned long src_pfn);
extern int arch_swsusp_write(unsigned int flags);
extern int swsusp_arch_restore_cpu(void);

struct saved_context {
	u32 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;	//0x00
	u32 usr_sp, usr_lr;						//0x34
	u32 svr_sp, svr_lr, svr_spsr;					//0x3c
	u32 abt_sp, abt_lr, abt_spsr;					//0x48
	u32 irq_sp, irq_lr, irq_spsr;					//0x54
	u32 und_sp, und_lr, und_spsr;					//0x60
	u32 fiq_sp, fiq_lr, fiq_spsr;					//0x6c
	u32 fiq_r8, fiq_r9, fiq_r10, fiq_r11, fiq_r12;			//0x78
	u32 cp15_control;						//0x8c
	u32 cp15_ttbr;							//0x90
	u32 cp15_dacl;							//0x94
	u32 rev[26];							//0x98
} __attribute__((packed));

#endif /* _ASM_ARM_SUSPEND_H */

