/*
 * arch/arm/plat-ambarella/generic/reglock.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include <mach/hardware.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX		"ambarella_config."

/* ==========================================================================*/
DEFINE_SPINLOCK(ambarella_register_lock);
unsigned long ambarella_register_flags;
u32 amb_reglock_count = 0;

/* ==========================================================================*/
EXPORT_SYMBOL(ambarella_register_lock);
EXPORT_SYMBOL(ambarella_register_flags);
EXPORT_SYMBOL(amb_reglock_count);
module_param (amb_reglock_count, uint, S_IRUGO);

