/*
 * arch/arm/plat-ambarella/include/plat/uhc.h
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

#ifndef __PLAT_AMBARELLA_UHC_H
#define __PLAT_AMBARELLA_UHC_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#define USB1_IS_HOST	(1 << 15)

struct ambarella_uhc_controller {
	void		(*enable_host)(struct ambarella_uhc_controller *pdata);
	void		(*disable_host)(void);
	int		usb1_is_host;
};

/* ==========================================================================*/
extern struct platform_device			ambarella_ehci0;
extern struct platform_device			ambarella_ohci0;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

