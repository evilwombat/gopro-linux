/*
 * arch/arm/mach-ambarella/touch-tm1726.c
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c/tm1726.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/board.h>

/* ==========================================================================*/
static int ambarella_tm1726_get_pendown_state(void)
{
	if (ambarella_gpio_get(
		ambarella_board_generic.touch_panel_irq.irq_gpio))
		return 0;
	else
		return 1;
}

static void ambarella_tm1726_clear_penirq(void)
{
	struct irq_desc		*touch_desc;
	struct irq_chip		*touch_chip = NULL;

	touch_desc = irq_to_desc(ambarella_board_generic.touch_panel_irq.irq_line);
	if (touch_desc)
		touch_chip = get_irq_desc_chip(touch_desc);
	if (touch_chip && touch_chip->irq_ack)
		touch_chip->irq_ack(&touch_desc->irq_data);
}

static int ambarella_tm1726_init_platform_hw(void)
{
	ambarella_gpio_config(ambarella_board_generic.touch_panel_irq.irq_gpio,
		ambarella_board_generic.touch_panel_irq.irq_gpio_mode);
	set_irq_type(ambarella_board_generic.touch_panel_irq.irq_line,
		ambarella_board_generic.touch_panel_irq.irq_type);
	ambarella_tm1726_clear_penirq();

	ambarella_set_gpio_reset(&ambarella_board_generic.touch_panel_reset);

	return 0;
}

static void ambarella_tm1726_exit_platform_hw(void)
{
}

static struct tm1726_platform_data ambarella_tm1726_pdata = {
	.fix = {
		[TM1726_FAMILY_0] = {
			.x_invert	= 0,
			.y_invert	= 0,
			.x_rescale	= 0,
			.y_rescale	= 0,
			.x_min		= 0,
			.x_max		= 1123,
			.y_min		= 0,
			.y_max		= 1872,

			.family_code	= 0x00,
		},

		[TM1726_FAMILY_1] = {
			.x_invert	= 0,
			.y_invert	= 0,
			.x_rescale	= 0,
			.y_rescale	= 0,
			.x_min		= 0,
			.x_max		= 1123,
			.y_min		= 0,
			.y_max		= 1872,

			.family_code	= 0x26,
		},

		[TM1726_FAMILY_2] = {
			.x_invert	= 0,
			.y_invert	= 0,
			.x_rescale	= 0,
			.y_rescale	= 0,
			.x_min		= 0,
			.x_max		= 1123,
			.y_min		= 0,
			.y_max		= 1872,

			.family_code	= 0xc1,
		},
	},
	.get_pendown_state	= ambarella_tm1726_get_pendown_state,
	.clear_penirq		= ambarella_tm1726_clear_penirq,
	.init_platform_hw	= ambarella_tm1726_init_platform_hw,
	.exit_platform_hw	= ambarella_tm1726_exit_platform_hw
};

struct i2c_board_info ambarella_tm1726_board_info = {
	.type		= "tm1726",
	.addr		= 0x22,
	.platform_data	= &ambarella_tm1726_pdata,
};

