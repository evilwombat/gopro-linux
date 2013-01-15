/*
 * arch/arm/plat-ambarella/include/mach/hardware.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

/* ==========================================================================*/
#include <mach/memory.h>
#include <ambhw/ambhw.h>

#include <plat/gpio.h>
#include <plat/atag.h>
#include <plat/event.h>
#include <plat/irq.h>
#include <plat/pm.h>
#include <plat/pwm.h>
#include <plat/rct.h>
#include <plat/reglock.h>
#if defined(CONFIG_PLAT_AMBARELLA_CORTEX)
#include <plat/cortex.h>
#endif

/* ==========================================================================*/
#define	AMBA_DEV_MAJOR			(248)
#define	AMBA_DEV_MINOR_PUBLIC_START	(128)
#define	AMBA_DEV_MINOR_PUBLIC_END	(240)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/
extern u64 ambarella_dmamask;

/* ==========================================================================*/
extern int ambarella_create_proc_dir(void);
extern struct proc_dir_entry *get_ambarella_proc_dir(void);

#endif
/* ==========================================================================*/

#endif

