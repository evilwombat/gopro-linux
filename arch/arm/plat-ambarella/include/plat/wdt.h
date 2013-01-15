/*
 * arch/arm/plat-ambarella/include/plat/wdt.h
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

#ifndef __PLAT_AMBARELLA_WDT_H
#define __PLAT_AMBARELLA_WDT_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_wdt_controller {
	u32	(*get_pll)(void);
	void	(*start)(u32 mode);
};

/* ==========================================================================*/
extern struct platform_device			ambarella_wdt0;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

