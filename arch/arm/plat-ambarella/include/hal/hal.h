/*
 * arch/arm/plat-ambarella/include/hal/hal.h
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

#ifndef __PLAT_AMBARELLA_HAL_H
#define __PLAT_AMBARELLA_HAL_H

/* ==========================================================================*/
#ifndef __ASM_ARCH_HARDWARE_H
#error "include <mach/hardware.h> first"
#endif

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_MMAP_NEW)
#define DEFAULT_HAL_START		(0x000a0000)
#else
#define DEFAULT_HAL_START		(0xc00a0000)
#endif
#define DEFAULT_HAL_BASE		(0xfee00000)
#define DEFAULT_HAL_SIZE		(0x00030000)

#if defined(CONFIG_BOSS_MULTIPLE_CORE)
#include <linux/aipc/ipc_slock.h>
#define AMBARELLA_BOSS_HAL_LOCK() \
	ipc_spin_lock(IPC_SLOCK_ID_HAL, IPC_SLOCK_POS_HAL)

#define AMBARELLA_BOSS_HAL_UNLOCK() \
	ipc_spin_unlock(IPC_SLOCK_ID_HAL, IPC_SLOCK_POS_HAL)
#else
#define AMBARELLA_BOSS_HAL_LOCK()
#define AMBARELLA_BOSS_HAL_UNLOCK()
#endif

#if (CHIP_REV == I1)
#define AMBARELLA_HAL_OS_LOCK()	\
	do {	\
		AMBARELLA_REG_LOCK();	\
		AMBARELLA_INC_REGLOCK_COUNT();	\
		AMBARELLA_BOSS_HAL_LOCK(); \
	} while(0)


#define AMBARELLA_HAL_OS_UNLOCK()	\
	do {	\
		AMBARELLA_BOSS_HAL_UNLOCK(); \
		AMBARELLA_REG_UNLOCK();	\
	} while(0)

#else
#define AMBARELLA_HAL_OS_LOCK()
#define AMBARELLA_HAL_OS_UNLOCK()
#endif

#endif /* CONFIG_PLAT_AMBARELLA_SUPPORT_HAL */

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)

#include <hal/header.h>
#if (CHIP_REV == A7)
#include <hal/a7/ambhal.h>
#elif (CHIP_REV == A7L)
#include <hal/a7l/ambhal.h>
#elif (CHIP_REV == A5S)
#include <hal/a5s/ambhal.h>
#elif (CHIP_REV == I1)
#include <hal/i1/ambhal.h>
#else
#error "Undefined CHIP_REV, Can't support HAL!"
#endif

struct ambarella_mem_hal_desc {
	u32 physaddr;
	u32 size;
	u32 virtual;
	u32 remapped;
	u32 inited;
};

/* ==========================================================================*/
extern void set_ambarella_hal_invalid(void);
extern void *get_ambarella_hal_vp(void);

/* ==========================================================================*/
#define HAL_BASE_VP			(get_ambarella_hal_vp())

#endif /* CONFIG_PLAT_AMBARELLA_SUPPORT_HAL */
#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

