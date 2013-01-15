/*
 * arch/arm/power/hibernate.c
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

#include <asm/suspend.h>

extern struct pbe *restore_pblist;
extern const void __nosave_begin, __nosave_end;
extern int swsusp_arch_restore_image(struct pbe *restore_pblist);

int swsusp_arch_resume(void)
{
	swsusp_arch_restore_image(restore_pblist);

	return 1;
}

int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = PFN_DOWN(__pa(&__nosave_begin));
	unsigned long nosave_end_pfn = PFN_UP(__pa(&__nosave_end));

	if ((pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn))
		return 1;

	return arch_pfn_is_nosave(pfn);
}

