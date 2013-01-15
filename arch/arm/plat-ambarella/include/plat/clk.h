/*
 * arch/arm/plat-ambarella/generic/clk.c
 *
 * History:
 *	2011/07/05 - [Zhenwu Xue] Initial Version
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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

#include <linux/clk.h>

#define PLL_REG_UNAVAILABLE	0
#define PLL_RESET_THRESHOLD	(50 * 1000)

struct clk_ops {
	int			(*enable)(struct clk *c);
	int			(*disable)(struct clk *c);
	unsigned long		(*get_rate)(struct clk *c);
	unsigned long		(*round_rate)(struct clk *c, unsigned long rate);
	int			(*set_rate)(struct clk *c, unsigned long rate);
	int			(*set_parent)(struct clk *c, struct clk *parent);
};

struct clk {
	struct list_head	list;
	struct clk		*parent;
	const char		*name;
	int			usage;
	unsigned long		rate;
	unsigned int		ctrl_reg;
	unsigned int		pres_reg;
	unsigned int		post_reg;
	unsigned int		frac_reg;
	unsigned int		ctrl2_reg;
	unsigned int		ctrl3_reg;
	unsigned int		lock_reg;
	unsigned int		lock_bit;
	unsigned int		divider;
	struct clk_ops		*ops;
};

extern int ambarella_init_clk(void);

