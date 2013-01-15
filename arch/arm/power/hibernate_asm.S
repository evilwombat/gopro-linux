/*
 * arch/arm/power/hibernate_asm.S
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

#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>

.text

ENTRY(swsusp_arch_suspend)
	ldr	r3, arch_saved_context
	add	r0, r3, #SAVED_CONTEXT_R0
	stmea	r0, {r0-r12}
	add	r0, r3, #SAVED_CONTEXT_SVR
	mov	r1, sp
	mov	r2, lr
	mrs	r3, spsr
	stmea	r0, {r1-r3}
	b	swsusp_save

ENTRY(swsusp_arch_restore_image)
	mov	r4, r0
	adr	r5, arch_debug_context
1000:
	ldr	r0, [r4, #PBE_ORIG_ADDRESS]
	ldr	r1, [r4, #PBE_ADDRESS]
	mov	r2, #PAGE_SZ
	stmea	r5, {r0-r2, r4}
#if 0
	bl	memcpy
#else
	mov	r6, #4
2000:
	ldr	r3, [r1], r6
	str	r3, [r0], r6
	subs	r2, r2, r6
	bne	2000b
#endif
	ldr	r4, [r4, #PBE_NEXT]
	cmp	r4, #0x00
	bne	1000b

ENTRY(swsusp_arch_restore_cpu)
	ldr	r3, arch_saved_context
	add	r0, r3, #SAVED_CONTEXT_R0
	add	r0, r0, #16
	ldmfd	r0, {r4-r12}
	add	r0, r3, #SAVED_CONTEXT_SVR
	ldmfd	r0, {r1-r3}
	mov	sp, r1
	mov	lr, r2
	msr	spsr_cxsf, r3
	mov	r0, #0x00
	mov	pc, lr

arch_saved_context:
	.long	arch_arm_context
arch_debug_context:
	.long	0x00
	.long	0x00
	.long	0x00
	.long	0x00

