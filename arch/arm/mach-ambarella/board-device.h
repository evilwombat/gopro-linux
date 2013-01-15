/*
 * arch/arm/mach-ambarella/board-device.h
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

/* ==========================================================================*/
extern struct i2c_board_info ambarella_ak4183_board_info;
extern struct i2c_board_info ambarella_chacha_mt4d_board_info;
extern struct i2c_board_info ambarella_tm1510_board_info;
extern struct i2c_board_info ambarella_tm1726_board_info;
extern struct i2c_board_info ambarella_tm1927_board_info;
extern struct i2c_board_info ambarella_nt11001_board_info;
extern struct i2c_board_info ambarella_ft540_board_info;
extern struct i2c_board_info ambarella_isl12022m_board_info;

extern struct i2c_board_info ambarella_board_vin_infos[2];

extern struct i2c_board_info ambarella_board_hdmi_info;

extern	struct platform_device lcd_1p3831;

extern struct wm831x_pdata elephant_wm8310_pdata;

extern int ambarella_init_wm8994(void);
extern int ambarella_init_wm8737(u8 i2c_bus_num, u8 i2c_addr);

