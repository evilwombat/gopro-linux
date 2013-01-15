/*
 * arch/arm/mach-ambarella/codec-wm8737.c
 *
 * Author: Louis Sun <lysun@ambarella.com>
 *
 * History:
 *	2011/05/11 - [Louis Sun] Created file
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
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/i2c.h>
#include <linux/gpio.h>

static struct i2c_board_info ambarella_wm8737_board_info = {
	I2C_BOARD_INFO("wm8737", 0x1A),   /* 1A is 7-bit I2C addr */
	.platform_data	= NULL,
};

int ambarella_init_wm8737(u8 i2c_bus_num, u8 i2c_addr) 
{
	if (i2c_addr)
		ambarella_wm8737_board_info.addr = i2c_addr;

	i2c_register_board_info(i2c_bus_num, &ambarella_wm8737_board_info, 1);
	return 0;
}

