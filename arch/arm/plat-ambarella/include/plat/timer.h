/*
 * arch/arm/plat-ambarella/include/plat/timer.h
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

#ifndef __PLAT_AMBARELLA_TIMER_H
#define __PLAT_AMBARELLA_TIMER_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/
extern struct sys_timer			ambarella_timer;

/* ==========================================================================*/
extern int ambarella_init_tm(void);

extern u32 ambarella_timer_suspend(u32 level);
extern u32 ambarella_timer_resume(u32 level);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

