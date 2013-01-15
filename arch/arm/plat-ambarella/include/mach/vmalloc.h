/*
 * arch/arm/plat-ambarella/include/mach/vmalloc.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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

#ifndef __ASM_ARCH_VMALLOC_H
#define __ASM_ARCH_VMALLOC_H

/* ==========================================================================*/
#if defined(CONFIG_VMSPLIT_3G)
#define VMALLOC_END			UL(0xe0000000)
#else
#define VMALLOC_END			UL(0xb0000000)
#endif

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

