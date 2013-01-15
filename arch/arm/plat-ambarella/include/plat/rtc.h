/*
 * arch/arm/plat-ambarella/include/plat/rtc.h
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

#ifndef __PLAT_AMBARELLA_RTC_H
#define __PLAT_AMBARELLA_RTC_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct ambarella_rtc_controller {
	u8					pos0;
	u8					pos1;
	u8					pos2;
	int					(*check_capacity)(u32);
	u32					(*check_status)(void);
	void					(*set_curt_time)(u32);
	u32					(*get_curt_time)(void);
	void					(*set_alat_time)(u32);
	u32					(*get_alat_time)(void);
	u32					reset_delay;
};
#define AMBA_RTC_PARAM_CALL(id, arg, perm, param_ops_rtcpos) \
	module_param_cb(rtc##id##_pos0, &param_ops_byte, &(arg.pos0), perm); \
	module_param_cb(rtc##id##_pos1, &param_ops_byte, &(arg.pos1), perm); \
	module_param_cb(rtc##id##_pos2, &param_ops_byte, &(arg.pos2), perm); \
	module_param_cb(rtc##id##_setpos, &param_ops_rtcpos, NULL, 0200)

/* ==========================================================================*/
extern struct platform_device			ambarella_rtc0;

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

