/*
 * arch/arm/mach-ambarella/vin.c
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

#include <linux/kernel.h>
#include <linux/i2c.h>

#include <mach/hardware.h>
#include <plat/idc.h>

/* ==========================================================================*/
struct i2c_board_info ambarella_board_vin_infos[2] = {
	[0] = {
		.type			= "amb_vin",
		.addr			= 0x02,
	},
	[1] = {
		.type			= "amb_vin1",
		.addr			= 0x03,
	},
};

