/*
 * arch/arm/mach-ambarella/lcd-1p3831.c
 *
 * Author: Zhenwu Xue <zwxue@ambarella.com>
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
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <mach/board.h>
#include <plat/spi.h>
#include <plat/gpio.h>

#include <video/platform_lcd.h>

/* ========================================================================== */

typedef struct {
	u16		addr;
	u8		val;
} reg_t;

static const reg_t TRUELY_1P3831_REGS[] =
{
	/* Power Control */
	{0xc000, 0x86},
	{0xc001, 0x00},
	{0xc002, 0x86},
	{0xc003, 0x00},
	{0xc100, 0x40},
	{0xc200, 0x12},
	{0xc202, 0x42},

	/* Gamma Control */
	{0xe000, 0x0e},
	{0xe001, 0x2a},
	{0xe002, 0x33},
	{0xe003, 0x3b},
	{0xe004, 0x1e},
	{0xe005, 0x30},
	{0xe006, 0x64},
	{0xe007, 0x3f},
	{0xe008, 0x21},
	{0xe009, 0x27},
	{0xe00a, 0x88},
	{0xe00b, 0x14},
	{0xe00c, 0x35},
	{0xe00d, 0x56},
	{0xe00e, 0x79},
	{0xe00f, 0xb8},
	{0xe010, 0x55},
	{0xe011, 0x57},
	{0xe100, 0x0e},
	{0xe101, 0x2a},
	{0xe102, 0x33},
	{0xe103, 0x3b},
	{0xe104, 0x1e},
	{0xe105, 0x30},
	{0xe106, 0x64},
	{0xe107, 0x3f},
	{0xe108, 0x21},
	{0xe109, 0x27},
	{0xe10a, 0x88},
	{0xe10b, 0x14},
	{0xe10c, 0x35},
	{0xe10d, 0x56},
	{0xe10e, 0x79},
	{0xe10f, 0xb8},
	{0xe110, 0x55},
	{0xe111, 0x57},
	{0xe200, 0x0e},
	{0xe201, 0x2a},
	{0xe202, 0x33},
	{0xe203, 0x3b},
	{0xe204, 0x1e},
	{0xe205, 0x30},
	{0xe206, 0x64},
	{0xe207, 0x3f},
	{0xe208, 0x21},
	{0xe209, 0x27},
	{0xe20a, 0x88},
	{0xe20b, 0x14},
	{0xe20c, 0x35},
	{0xe20d, 0x56},
	{0xe20e, 0x79},
	{0xe20f, 0xb8},
	{0xe210, 0x55},
	{0xe211, 0x57},
	{0xe300, 0x0e},
	{0xe301, 0x2a},
	{0xe302, 0x33},
	{0xe303, 0x3b},
	{0xe304, 0x1e},
	{0xe305, 0x30},
	{0xe306, 0x64},
	{0xe307, 0x3f},
	{0xe308, 0x21},
	{0xe309, 0x27},
	{0xe30a, 0x88},
	{0xe30b, 0x14},
	{0xe30c, 0x35},
	{0xe30d, 0x56},
	{0xe30e, 0x79},
	{0xe30f, 0xb8},
	{0xe310, 0x55},
	{0xe311, 0x57},
	{0xe400, 0x0e},
	{0xe401, 0x2a},
	{0xe402, 0x33},
	{0xe403, 0x3b},
	{0xe404, 0x1e},
	{0xe405, 0x30},
	{0xe406, 0x64},
	{0xe407, 0x3f},
	{0xe408, 0x21},
	{0xe409, 0x27},
	{0xe40a, 0x88},
	{0xe40b, 0x14},
	{0xe40c, 0x35},
	{0xe40d, 0x56},
	{0xe40e, 0x79},
	{0xe40f, 0xb8},
	{0xe410, 0x55},
	{0xe411, 0x57},
	{0xe500, 0x0e},
	{0xe501, 0x2a},
	{0xe502, 0x33},
	{0xe503, 0x3b},
	{0xe504, 0x1e},
	{0xe505, 0x30},
	{0xe506, 0x64},
	{0xe507, 0x3f},
	{0xe508, 0x21},
	{0xe509, 0x27},
	{0xe50a, 0x88},
	{0xe50b, 0x14},
	{0xe50c, 0x35},
	{0xe50d, 0x56},
	{0xe50e, 0x79},
	{0xe50f, 0xb8},
	{0xe510, 0x55},
	{0xe511, 0x57},

	/* RGB Interface Format */
	{0x3a00, 0x07},
	{0x3b00, 0x03},
};

static int skip_lcd_1p3831_init = 0;
static int lcd_1p3831_power = 0;

/* ========================================================================== */
static void truely_1p3831_write_cmd(u16 cmd)
{
	amba_spi_write_t	write;
	u16			data[2];

	/* spi write */
	write.bus_id		= ambarella_board_generic.lcd_spi_hw.bus_id;
	write.cs_id		= ambarella_board_generic.lcd_spi_hw.cs_id;
	write.buffer		= (u8 *)&data;
	write.n_size		= sizeof(data);

	data[0] = 0x2000 | (cmd >> 8);
	data[1] = 0x0000 | (cmd & 0xff);
	ambarella_spi_write(&ambarella_board_generic.lcd_spi_cfg, &write);
}

static void truely_1p3831_write_cmd_data(reg_t reg)
{
	amba_spi_write_t	write;
	u16			data[3];

	/* spi write */
	write.bus_id		= ambarella_board_generic.lcd_spi_hw.bus_id;
	write.cs_id		= ambarella_board_generic.lcd_spi_hw.cs_id;
	write.buffer		= (u8 *)&data;
	write.n_size		= sizeof(data);

	data[0] = 0x2000 | (reg.addr >> 8);
	data[1] = 0x0000 | (reg.addr & 0xff);
	data[2] = 0x4000 | reg.val;
	ambarella_spi_write(&ambarella_board_generic.lcd_spi_cfg, &write);
}

static void lcd_1p3831_set_power(struct plat_lcd_data *pdata,
	unsigned int power)
{
	int			i;

	if (power == lcd_1p3831_power) {
		return;
	}

	if (power) {
		if (skip_lcd_1p3831_init) {
			skip_lcd_1p3831_init = 0;
		} else {
			ambarella_set_gpio_output(
				&ambarella_board_generic.lcd_power, 1);
			ambarella_set_gpio_reset(
				&ambarella_board_generic.lcd_reset);
			truely_1p3831_write_cmd(0x1100);
			mdelay(120);
			for (i = 0; i < sizeof(TRUELY_1P3831_REGS) /
				sizeof(reg_t); i++) {
				truely_1p3831_write_cmd_data(
					TRUELY_1P3831_REGS[i]);
			}
			truely_1p3831_write_cmd(0x2900);
			ambarella_set_gpio_output(
				&ambarella_board_generic.lcd_backlight, 1);
			pr_info("Power on 1p3831\n");
		}
	} else {
		pr_info("Power off 1p3831\n");
		ambarella_set_gpio_output(
			&ambarella_board_generic.lcd_power, 0);
		ambarella_set_gpio_output(
			&ambarella_board_generic.lcd_backlight, 0);
		skip_lcd_1p3831_init = 0;
	}
	lcd_1p3831_power = power;
}

static int lcd_1p3831_match_fb(struct plat_lcd_data *pdata,
	struct fb_info *pinfo)
{
	return 1;
}

struct plat_lcd_data lcd_1p3831_pdata = {
	.set_power	= lcd_1p3831_set_power,
	.match_fb	= lcd_1p3831_match_fb,
};

struct platform_device lcd_1p3831 = {
	.name		= "platform-lcd",
	.id		= 0,
	.dev		= {
		.platform_data		= &lcd_1p3831_pdata,
	},
};

static int __init early_skip_lcd_1p3831_init(char *p)
{
	skip_lcd_1p3831_init = 1;
	return 0;
}
early_param("skip_lcd_1p3831_init", early_skip_lcd_1p3831_init);

