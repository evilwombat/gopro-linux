/*
 * arch/arm/plat-ambarella/include/plat/ir.h
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

#ifndef __PLAT_AMBARELLA_IR_H
#define __PLAT_AMBARELLA_IR_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

enum ambarella_ir_protocol {
	AMBA_IR_PROTOCOL_NEC = 0,
	AMBA_IR_PROTOCOL_PANASONIC = 1,
	AMBA_IR_PROTOCOL_SONY = 2,
	AMBA_IR_PROTOCOL_PHILIPS = 3,
	AMBA_IR_PROTOCOL_END
};

struct ambarella_ir_controller {
	void					(*set_pll)(void);
	u32					(*get_pll)(void);

	int					protocol;
	int					debug;
};
#define AMBA_IR_PARAM_CALL(arg, perm) \
	module_param_cb(ir_protocol, &param_ops_int, &arg.protocol, perm); \
	module_param_cb(ir_debug, &param_ops_int, &arg.debug, perm)

/* ==========================================================================*/
extern struct platform_device			ambarella_ir0;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

