/*
 * arch/arm/power/sleep.c
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

#include <linux/suspend.h>
#include <linux/stddef.h>

#include <asm/suspend.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

struct saved_context arch_arm_context;

void save_processor_state(void)
{
	__memzero(&arch_arm_context, sizeof(arch_arm_context));
}
EXPORT_SYMBOL(save_processor_state);

void restore_processor_state(void)
{
	flush_tlb_all();
	flush_cache_all();
}
EXPORT_SYMBOL(restore_processor_state);

