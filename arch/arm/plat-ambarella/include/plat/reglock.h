/*
 * arch/arm/plat-ambarella/include/plat/reglock.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#ifndef __PLAT_AMBARELLA_REGISTER_LOCK_H
#define __PLAT_AMBARELLA_REGISTER_LOCK_H

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#ifdef CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK
#include <linux/spinlock.h>
extern spinlock_t ambarella_register_lock;
extern unsigned long ambarella_register_flags;
extern u32 amb_reglock_count;

#define AMBARELLA_REG_LOCK()		\
	spin_lock_irqsave(&ambarella_register_lock, ambarella_register_flags)
#define AMBARELLA_REG_UNLOCK()		\
	spin_unlock_irqrestore(&ambarella_register_lock, ambarella_register_flags)
#define AMBARELLA_INC_REGLOCK_COUNT()	\
	amb_reglock_count++
#else
#define AMBARELLA_REG_LOCK()
#define AMBARELLA_REG_UNLOCK()
#define AMBARELLA_INC_REGLOCK_COUNT()
#endif /* CONFIG_PLAT_AMBARELLA_ADD_REGISTER_LOCK */
#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

