/*
 * arch/arm/plat-ambarella/include/plat/pwm.h
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

#ifndef __PLAT_AMBARELLA_PWM_H
#define __PLAT_AMBARELLA_PWM_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/
struct ambarella_pwm_info {
	unsigned int period_ns;
	unsigned int max_duty;
};

#define AMBA_PWM_MODULE_PARAM_CALL(name_prefix, arg, perm) \
	module_param_cb(name_prefix##period_ns, &param_ops_int, &(arg.period_ns), perm); \
	module_param_cb(name_prefix##max_duty, &param_ops_int, &(arg.max_duty), perm)

extern struct platform_device ambarella_pwm_platform_device0;
extern struct platform_device ambarella_pwm_platform_device1;
extern struct platform_device ambarella_pwm_platform_device2;
extern struct platform_device ambarella_pwm_platform_device3;
extern struct platform_device ambarella_pwm_platform_device4;

/* ==========================================================================*/
extern int ambarella_init_pwm(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

