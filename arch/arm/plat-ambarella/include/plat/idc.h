/*
 * arch/arm/plat-ambarella/include/plat/idc.h
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

#ifndef __PLAT_AMBARELLA_IDC_H
#define __PLAT_AMBARELLA_IDC_H

/* ==========================================================================*/
#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || (CHIP_REV == A5L)
#define IDC_SUPPORT_PIN_MUXING_FOR_HDMI         1
#else
#define IDC_SUPPORT_PIN_MUXING_FOR_HDMI         0
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1)
#define IDC_SUPPORT_INTERNAL_MUX	1
#else
#define IDC_SUPPORT_INTERNAL_MUX	0
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == A2S) || (CHIP_REV == A2M)
#define IDC_PORTS_USING_INTERNAL_MUX 	1
#elif (CHIP_REV == A5L)
#define IDC_PORTS_USING_INTERNAL_MUX	2
#else
#define IDC_PORTS_USING_INTERNAL_MUX 	0
#endif

#define IDC_ENR_REG_ENABLE		(0x01)
#define IDC_ENR_REG_DISABLE		(0x00)

#define IDC_CTRL_STOP			(0x08)
#define IDC_CTRL_START			(0x04)
#define IDC_CTRL_IF			(0x02)
#define IDC_CTRL_ACK			(0x01)
#define IDC_CTRL_CLS			(0x00)

#define IDC_FIFO_BUF_SIZE		(63)

#define IDC_FMCTRL_STOP			(0x08)
#define IDC_FMCTRL_START		(0x04)
#define IDC_FMCTRL_IF			(0x02)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_idc_platform_info {
	int					clk_limit;	//Hz
	int					bulk_write_num;
	unsigned int				i2c_class;
#if defined(CONFIG_AMBARELLA_IPC)
	int					ipc_mutex_id;
#endif
	u32					(*get_clock)(void);
};
#define AMBA_IDC_PARAM_CALL(id, arg, perm) \
	module_param_cb(idc##id##_clk_limit, &param_ops_int, &(arg.clk_limit), perm); \
	module_param_cb(idc##id##_bulk_write_num, &param_ops_int, &(arg.bulk_write_num), perm)

/* ==========================================================================*/
extern struct platform_device			ambarella_idc0;
extern struct platform_device			ambarella_idc1;
extern struct platform_device			ambarella_i2cmux;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

