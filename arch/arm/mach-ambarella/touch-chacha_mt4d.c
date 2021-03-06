/*
 * arch/arm/mach-ambarella/touch-chacha_mt4d.c
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
#include <linux/init.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <mach/board.h>

#include <linux/i2c.h>
#include <linux/i2c/chacha_mt4d.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

/* ==========================================================================*/
static int ambarella_chacha_mt4d_get_pendown_state(void)
{
	if (ambarella_gpio_get(
		ambarella_board_generic.touch_panel_irq.irq_gpio))
		return 0;
	else
		return 1;
}

static void ambarella_chacha_mt4d_clear_penirq(void)
{
	struct irq_desc		*touch_desc;
	struct irq_chip		*touch_chip = NULL;

	touch_desc = irq_to_desc(ambarella_board_generic.touch_panel_irq.irq_line);
	if (touch_desc)
		touch_chip = get_irq_desc_chip(touch_desc);
	if (touch_chip && touch_chip->irq_ack)
		touch_chip->irq_ack(&touch_desc->irq_data);
}

static int ambarella_chacha_mt4d_init_platform_hw(void)
{
	ambarella_gpio_config(ambarella_board_generic.touch_panel_irq.irq_gpio,
		ambarella_board_generic.touch_panel_irq.irq_gpio_mode);
	set_irq_type(ambarella_board_generic.touch_panel_irq.irq_line,
		ambarella_board_generic.touch_panel_irq.irq_type);
	ambarella_chacha_mt4d_clear_penirq();
	ambarella_set_gpio_reset(&ambarella_board_generic.touch_panel_reset);

	return 0;
}

static void ambarella_chacha_mt4d_exit_platform_hw(void)
{
}

static struct chacha_mt4d_platform_data ambarella_chacha_mt4d_pdata = {
	.fix = {
		.x_invert = 0,
		.y_invert = 0,
		.x_rescale = 0,
		.y_rescale = 0,
		.x_min = 2,
		.x_max = 318,
		.y_min = 1,
		.y_max = 469,
	},
	.get_pendown_state = ambarella_chacha_mt4d_get_pendown_state,
	.clear_penirq = ambarella_chacha_mt4d_clear_penirq,
	.init_platform_hw = ambarella_chacha_mt4d_init_platform_hw,
	.exit_platform_hw = ambarella_chacha_mt4d_exit_platform_hw
};

struct i2c_board_info ambarella_chacha_mt4d_board_info = {
	.type = "chacha_mt4d",
	.addr = 0x40,
	.platform_data = &ambarella_chacha_mt4d_pdata,
};

